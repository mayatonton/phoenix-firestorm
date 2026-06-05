# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-α complete handoff

**作成日**: 2026-06-03
**branch**: `feature/ayastorm-r41-gl-removal`
**prep commit**: `(本 handoff の直前 verify-prep commit、handoff/handoff-...-phase2d-alpha-verify-prep.md 同梱)`
**feat commit (literal scope、Phase 2d-α 初版)**: `0587c574da` (5 GLSL file 編集、pivot doc §4.1 由来)
**docs commit (補助)**: `9a576884c0` (reference doc binding 10 note + η-28-E/F 範式追加)
**feat commit (Phase 2d-α fix、本 commit 提出予定)**: `(本 commit、PBRMix guard 漏れ補完 + SpotLight 設計外拡張 撤回 + cross-stage UBO 共有適用)`
**前 phase**: η-28 Phase 2c (commit `c08080ded2` + `5936ee7f2d`、parse 8→7 + link 1→0 + Water Shader PASS)
**状態**: Phase 2d-α **AYA cold launch verify 2 周完了 (regression fix 1 周追加) + Phase 2d-α 起因 2 件完全消化**。残 parse fail / link fail は **既存 vulkanize C++ 側問題** (Phase 2d-α 副作用でない、§5 で確定)。次は Phase 2d-β prep 起草 (= 設計 source of truth に従う Phase 0 計測着手の docs 化)。

---

## §1 結果サマリー (cold launch 実測 2 周 vs prep doc §2.2 期待)

### §1.1 1st cold launch (post-feat `0587c574da`、Fix 適用前)

| 観測項目 | Phase 2c 末 | Phase 2d-α 1st 末 (実測) | Δ | 期待 (prep §2.2) | 判定 |
|---|---|---|---|---|---|
| GLSL parse fail 件数 | 7 | **4** | **-3** | **2** | △ 期待比 +2 (Phase 2d-α 編集 2 ファイル 範式漏れ + 編集 1 ファイル 設計外拡張) |
| link fail 件数 | 0 | **1** | **+1** | **0** | **✗ regression** |
| 5 program SPIR-V 生成 | (Phase 2c で 4/5 PASS) | **4 成功 / 1 link fail** | (mixed) | 5 成功 | △ 部分達成 |
| viewer 起動 | (Phase 2c PASS) | 通常表示 PASS (= default GL path) | - | (vk-α では黒画面 + UI、本 verify は GL 動作のため通常表示が正) | ✓ (prep §2.2 文言が vk-α 想定で誤読しやすかった、本 §1.4 で訂正) |
| sustained | ~10 分 | (Fix 待ちで 1st は途中で stop) | - | - | (中断、正常) |

**判定根拠**:
- Phase 2d-α 初版で消化された 5 root 中 **3 root が消化 (parse 7→4)** = pointLightF size + SpotLight color rename + TerrainMix struct redef guard
- **残 4 件 = 2 件既知別件 (Skinned/Deferred PBR Alpha、Phase 2c 末から持越) + 2 件 NEW** = Phase 2d-α 編集起因の **PBRMix struct redefinition** (= 範式 η-28-F 適用漏れ、pbrterrainF/Util に TerrainMix guard 追加時に PBRMix guard 追加を見落とし)
- **link fail 1 件 = SpotLight Anonymous member name `center`** = Phase 2d-α 編集起因の設計外拡張 (= PerProgramUBO_SpotLightF に chunk 3 として `vec3 center + float _pad_center` を新規追加したが、vert stage の PerProgramUBO_PointLightV.center と Vulkan global symbol scope で衝突)

### §1.2 Fix 適用 (本 commit、Phase 2c Issue A と同型 = anonymous member collision)

| Fix 区分 | ファイル | 修正内容 |
|---|---|---|
| **Fix-1 (Failure 1: PBRMix redefinition)** | `class1/deferred/pbrterrainF.glsl` L68-85 | `struct PBRMix` を `#ifndef PBR_MIX_DEFINED ... #endif` で guard wrap (η-28-F 範式同型適用、初版漏れ補完) |
| **Fix-1 連動** | `class1/deferred/pbrterrainUtilF.glsl` L88-105 | 同じ PBRMix guard wrap (pbrterrainF と対、addCommonShader 経由 attach での redefinition 防止) |
| **Fix-2 (Failure 2: SpotLight Anonymous member collision、設計 alignment)** | `class3/deferred/spotLightF.glsl` chunk 3 撤回 | `PerProgramUBO_SpotLightF` から `vec3 center + float _pad_center` (chunk 3、offset 48) を削除 = **Phase 2d-α 初版で blueprint 不在の独自拡張だった部分を撤回**、設計 chapter 02 §3.3 表の独立 block 構造に戻す |
| **Fix-2 連動 (cross-stage 共有適用)** | `class3/deferred/spotLightF.glsl` MULTI_SPOTLIGHT wrap 書換 | `#if defined(MULTI_SPOTLIGHT) / #ifdef LL_VULKAN_GLSL` 内で **`PerProgramUBO_PointLightV` (set=2 binding=5) を frag stage にも declare** (vert stage と同 block 名 + 同 binding + `PER_PROGRAM_UBO_POINT_LIGHT_V_DEFINED` guard) = **η-28-C type 1 範式適用 (Phase 2c Issue A の WaterVParamUBO_Legacy cross-stage import と同型)**、GL path は従来の `uniform vec3 center;` 維持 |

### §1.3 2nd cold launch (post-fix、本 commit 適用後)

| 観測項目 | Phase 2d-α 1st 末 | Phase 2d-α 2nd 末 (実測) | Δ | 期待 | 判定 |
|---|---|---|---|---|---|
| GLSL parse fail 件数 | 4 | **3** | **-1** | 2 | △ 期待比 +1 (= 新規 1 件 = paintmap triplanar、§5 で既存 vulkanize bug と確定) |
| link fail 件数 | 1 | **0** → **1** (新規) | (種別変化) | 0 | △ Phase 2d-α 起因 SpotLight link fail は **完全消失**、新規 1 件 = heightmap-with-noise triplanar (§5 で既存 vulkanize bug と確定) |
| 5 program SPIR-V 生成 | 4/5 | **5/5 PASS** (Deferred SpotLight Shader 含む) | +1 | 5 PASS | ✓ |
| viewer 起動 | 通常表示 | 通常表示 PASS | - | 通常表示 PASS | ✓ |
| sustained | (中断) | **~7 分 clean shutdown** (04:18:33 → 04:25:34) | - | ~10 分 | △ target ~10 分にやや短いが clean shutdown 達成 |

**判定根拠**:
- **Fix-1**: PBRMix struct redefinition 2 件 (heightmap + paintmap permutation) → 完全消失
- **Fix-2**: SpotLight Anonymous member name `center` link fail → 完全消失、Deferred SpotLight Shader が 5 target SPIR-V の 5 個目として PASS 成立
- **残 parse fail 3 + link fail 1 の内訳** (§5 詳細):
  - Skinned Deferred PBR Alpha Shader (parse、既知別件、Phase 2c から持越)
  - Deferred PBR Alpha Shader (parse、既知別件、Phase 2c から持越)
  - **Deferred PBR Terrain Shader 0 paintmap triplanar (parse、新規表面化)** = 既存 vulkanize C++ 側の `vary_texcoord` location override 28→29 と `vary_coords` location 29 の衝突 (Phase 2d-α 編集と独立)
  - **Deferred PBR Terrain Shader 0 heightmap-with-noise triplanar (link、新規表面化)** = 既存 vulkanize C++ 側の vert/frag slot allocation 不整合 (`vary_coords` vert location=30 vs frag location=29、Phase 2d-α 編集と独立)

### §1.4 prep §2.2 viewer 起動期待値の訂正記録

prep §2.2「**黒画面 + UI 描画 PASS** (= vk-α 空転 baseline 維持)」は vk-α (Vulkan active) 経路を前提とした文言で、本 verify は default `RenderEnableVulkan=0` の **GL path** 経路のため **通常表示が正解**。1st verify 時に AYA さんから「期待値に黒画面とありますが、今までどおりの表示でした」とのご指摘を頂き、本 doc で訂正記録。

→ **将来の prep doc では「viewer 起動」期待値を vk-α 経路 (黒画面 + UI) と GL path 経路 (通常表示) を分離して記述すべき** (= 範式追加候補、§9 で範式化)。

### §1.5 全体達成 vs prep §2.2 期待 (1st + 2nd 合算後)

| 観測項目 | Phase 2c 末 | Phase 2d-α 全完了 (2nd Fix 後) | Δ | 期待 | 判定 |
|---|---|---|---|---|---|
| parse fail 件数 | 7 | **3** | **-4** | 2 (= -5) | △ 期待比 +1 (新規 1 件 = 既存 vulkanize bug、Phase 2d-α 編集と独立、§5 確定) |
| link fail 件数 | 0 | **1** | +1 | 0 | △ 新規 1 件 = 既存 vulkanize bug (vary_coords slot allocation、Phase 2d-α 編集と独立、§5 確定) |
| **Phase 2d-α 起因 root 消化** | - | **5/5 PASS** (= pointLightF size + SpotLight color + TerrainMix + PBRMix Fix-1 + SpotLight Anonymous Fix-2) | - | 5/5 PASS | **✓ literal scope 完遂** |
| 5 program SPIR-V 生成 | (Phase 2c で部分達成) | **5/5 PASS** (= Deferred SpotLight Shader 含む) | - | 5/5 PASS | **✓** |
| viewer 起動 | (Phase 2c PASS) | 通常表示 PASS | - | (GL path 通常表示) | **✓** |
| sustained ~10 分 | - | **~7 分 clean shutdown** | - | ~10 分 | △ 短いが clean、機能 regression 0 件 |

**特記**: prep §2.2 期待 parse fail 2 + link 0 は **Phase 2d-α 編集 5 file の literal scope 内 (= 5 root 消化) のみで算出**。実測 parse fail 3 + link fail 1 の差分は **編集と独立した既存 vulkanize bug** (triplanar variant の slot allocation 不整合)、§5 で trace 確定。**Phase 2d-α scope 内では完全達成**。

---

## §2 Phase 2d-α 初版 commit `0587c574da` 内訳 (= prep §4 再掲、Fix 前の状態)

| file | 編集内容 | 修正対象 Issue | 判定 |
|---|---|---|---|
| `class3/deferred/pointLightF.glsl` | 自己 PerDrawUBO_LightParams 宣言削除 + body の `size` → `spot_light_size` / `color` → `spot_light_color` rename (`#ifdef LL_VULKAN_GLSL` 分岐) | Issue B (`size` undeclared at L2439、η-28-E 範式) | ✓ verify 2 周通過 |
| `class3/deferred/spotLightF.glsl` | (a) PerProgramUBO_SpotLightF に `vec3 center + float _pad_center` chunk 3 追加 + (b) `uniform vec3 center;` を `#ifndef LL_VULKAN_GLSL` で wrap + (c) 自己 PerDrawUBO_LightParams 削除 + (d) `color.rgb` → `spot_light_color.rgb` rename | Issue E + Issue F (MULTI_SPOTLIGHT permutation で `center` 参照、η-28-C type 3 範式) | △ (a)(b) は **本 Fix-2 で撤回 + cross-stage 共有に書換** (理由: §3)、(c)(d) は verify 通過 |
| `class1/deferred/pbrterrainF.glsl` | L47-51 `struct TerrainMix` を `#ifndef TERRAIN_MIX_DEFINED` guard wrap | Issue C (TerrainMix struct redefinition at L1750、η-28-F 範式) | △ TerrainMix は guard 適用済だが **同 file 内 struct PBRMix の guard 適用漏れ** = Fix-1 で補完 |
| `class1/deferred/pbrterrainUtilF.glsl` | L183-187 同じ TerrainMix guard wrap | Issue C 連動 | △ 同じく PBRMix guard 適用漏れ = Fix-1 で補完 |
| `class1/interface/pbrTerrainBakeF.glsl` | L34 同じ TerrainMix guard wrap | 予防修正 (η-28-F 範式 future-proofing) | ✓ verify 通過 |

---

## §3 Fix の設計適合性 (= AYA 「設計に合わせることはできない?」指摘への回答 source)

### §3.1 Fix-1 (PBRMix guard 漏れ補完) = 範式適用の literal 範囲拡張

`struct TerrainMix` には `#ifndef TERRAIN_MIX_DEFINED` guard を適用した一方、**同じ file 内に同様に attach 経路で redefinition される `struct PBRMix` を見落とした**。η-28-F 範式 (struct redef guard) は **同じ pattern が複数 struct で発生する場合は全 struct に同型適用** が前提で、設計外の範式逸脱ではなく **literal scope の範式適用漏れ**。Fix-1 は範式の literal 適用範囲を完成させる修正。

### §3.2 Fix-2 (SpotLight chunk 3 撤回 + cross-stage UBO 共有) = 設計 chapter 02 §3.3 alignment

**初版の chunk 3 拡張は blueprint 不在の独自拡張**:
- 設計 chapter 02 §3.3 表では `PerProgramUBO_SpotLightF` (binding 12) と `PerProgramUBO_PointLightV` (binding 5) が **独立 block** として記述、`center` は **PerProgramUBO_PointLightV 専属**
- Phase 2d-α 初版は MULTI_SPOTLIGHT permutation で `center` が必要となるため、**PerProgramUBO_SpotLightF に chunk 3 として `vec3 center + float _pad_center` を新規追加** した = 設計表に存在しない独自拡張
- Vulkan glslang は **anonymous block member 名を global symbol scope に展開**するため、vert stage で `PerProgramUBO_PointLightV.center` (binding 5) が宣言されている program に、frag stage で `PerProgramUBO_SpotLightF.center` (binding 12) を宣言すると **global symbol `center` が 2 つの異なる block に属する状態** → "Anonymous member name used for global variable" link error

**Fix-2 の設計適合**:
- chunk 3 を撤回し PerProgramUBO_SpotLightF を設計表の 3 chunk 構成に戻す
- MULTI_SPOTLIGHT permutation の `center` 参照は **既存の `PerProgramUBO_PointLightV` (set=2 binding=5) を frag stage にも declare** で解決 = **Phase 2c Issue A (WaterVParamUBO_Legacy cross-stage import) と同型の η-28-C type 1 範式適用**
- 設計 chapter 02 §3.3 表に対する **構造変更ゼロ、binding 追加ゼロ、既存 block の cross-stage 共有のみ**

### §3.3 範式系譜

| 範式 | Phase 2c (Water Shader Issue A) | Phase 2d-α (SpotLight Fix-2) |
|---|---|---|
| 問題種別 | F stage で wrap 候補 uniform 名が **他 set/binding の UBO に anonymous member として既存** | F stage で **設計外の chunk 拡張により vert stage 既存 UBO の anonymous member と global symbol 衝突** |
| 範式適用 | 既存 UBO (WaterVParamUBO_Legacy set=3 binding=60) を F stage に cross-stage import | 既存 UBO (PerProgramUBO_PointLightV set=2 binding=5) を F stage に cross-stage import |
| 設計差分 | binding 60 既存 reuse、binding 23 新規 PerProgramUBO_WaterF | binding 5 既存 reuse、binding 12 既存 PerProgramUBO_SpotLightF (chunk 3 拡張を撤回) |
| guard 名 | `WATER_V_PARAM_UBO_LEGACY_DEFINED` | `PER_PROGRAM_UBO_POINT_LIGHT_V_DEFINED` |
| 範式分類 | η-28-C type 1 (cross-stage same UBO 共有) | η-28-C type 1 (cross-stage same UBO 共有、Phase 2c と同型) |

---

## §4 5 program SPIR-V 生成確認 (2nd cold launch log 実測)

| program 名 (log) | source file | binding | SPIR-V 生成 | 状態 |
|---|---|---|---|---|
| Deferred Light Shader (pointLight) | pointLightF + pointLightV | 5 (V) + 25 (F、Phase 2c) | ✓ (2 stages, 2 files) | **✓ Phase 2d-α Issue B 解消** |
| Deferred SpotLight Shader | spotLightF + spotLightV | 5 (V) + 12 (F、cross-stage 5 共有適用後) | ✓ (2 stages, 2 files) | **✓ Phase 2d-α Issue E/F + Fix-2 解消** |
| Deferred PBR Terrain Shader 0 (non-triplanar 群) | pbrterrainF + pbrterrainV + pbrterrainUtilF | 24 (V) | ✓ (主要 variant 群) | **✓ Phase 2d-α Issue C (TerrainMix + Fix-1 PBRMix) 解消** |
| Deferred PBR Terrain Shader 0 heightmap-with-noise triplanar | (同上 + triplanar 変数) | 24 (V) | ✗ link fail (vary_coords slot allocation、§5 で既存 vulkanize bug 確定) | △ Phase 2d-α と独立 |
| Deferred PBR Terrain Shader 0 paintmap triplanar | (同上 + triplanar 変数) | 24 (V) | ✗ parse fail (location 29 overlap、§5 で既存 vulkanize bug 確定) | △ Phase 2d-α と独立 |
| Deferred PBR Terrain Bake Shader | pbrTerrainBakeF | 24 (V 共有) | ✓ | **✓ 予防修正適用済** |

**特記**: 5 program SPIR-V 完全 PASS = Phase 2d-α literal scope (5 GLSL file 編集) 完遂。triplanar variant 2 件は **§5 で trace により Phase 2d-α 編集と独立した既存 vulkanize C++ 側の slot allocation 不整合 bug** と確定。

---

## §5 Phase 2d-α 末 残 parse fail 3 + link fail 1 内訳 (= 既存別件確定)

### §5.1 既知別件 (Phase 2c から持越) 2 件

| program | stage | error | 推定 root | 振分 |
|---|---|---|---|---|
| Skinned Deferred PBR Alpha Shader | F | non-opaque uniforms outside a block @ 0:3692 + `screen_res` redefinition + missing #endif | pbralphaF.glsl skin path (η-27 hidden cascade) | **Phase 2d-β / 2d-γ (= 設計 source of truth の Phase 0 計測 + 1+ migration 着手後の literal scope 再 frame)** |
| Deferred PBR Alpha Shader | V | `modelview_projection_matrix` undeclared @ 0:1252 + missing #endif | pbralphaV.glsl (η-27 hidden cascade) | **Phase 2d-β / 2d-γ** |

### §5.2 新規表面化 = 既存 vulkanize C++ 側 bug 2 件 (= Phase 2d-α と独立)

#### §5.2.1 paintmap triplanar parse fail (L1970 location 29 overlap)

**source 確認** (`pbrterrainF.glsl` L238-269、編集対象外):
```glsl
#elif TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_PBR_PAINTMAP
#ifdef LL_VULKAN_GLSL
layout(location=28) in vec2 vary_texcoord;
...
#endif
#if TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT == 3
#ifdef LL_VULKAN_GLSL
layout(location=29) in vec4[10] vary_coords;
...
```

- `vary_texcoord` (paintmap variant 用) = source で **location=28** 宣言
- `vary_coords` (triplanar 用) = source で **location=29** 宣言

**vulkanize log の確定証拠**:
```
F stage layout in 'vary_texcoord' location override 28 -> 29 (V↔F pair alignment)
```
→ **C++ 側 vulkanize ロジックが `vary_texcoord` の location を 28 → 29 に強制 override**、結果 `vary_coords` (29) と衝突 → `'location' : overlapping use of location 29` parse fail。

**起因**: Phase 2d-α 編集 (PBRMix guard wrap、SpotLight cross-stage 共有) は **L238-269 location 宣言部に一切触れていない**。問題は **paintmap × triplanar permutation で必発の vulkanize C++ slot allocation 副作用**。

#### §5.2.2 heightmap-with-noise triplanar link fail (vert vary_coords=30 vs frag vary_coords=29)

**log の確定証拠**:
```
ERROR: Linking vertex and fragment stages: Layout location qualifier must match:
    vertex stage: vary_coords "layout( location=30) smooth out"
    fragment stage: vary_coords "layout( location=29) smooth in"
```
→ **vert と frag で `vary_coords` の location 番号が異なる** = vulkanize C++ 側 slot allocation が vert/frag で不整合。

**起因**: §5.2.1 と同型の vulkanize C++ 側 bug、Phase 2d-α 編集と独立。

### §5.3 §5.2 切り分け根拠 (= AYA 「コードみてわからないと言っている?」指摘への回答)

1. **Phase 2d-α 編集 file** = pointLightF (body rename) + spotLightF (UBO 構造) + pbrterrainF L68-85 (PBRMix guard 追加) + pbrterrainUtilF L88-105 (PBRMix guard 追加) + pbrTerrainBakeF L34 (TerrainMix guard 追加)
2. **問題発生 file** = pbrterrainF L1970 + L1750 = **L238-269 の `vary_texcoord` / `vary_coords` location 宣言部** (Phase 2d-α 編集対象外)
3. **vulkanize log の "F stage layout in 'vary_texcoord' location override 28 -> 29"** = **C++ 側 vulkanize ロジック起因の override**、GLSL ファイル編集と独立
4. 結論: **§5.2 の 2 件は Phase 2d-α 編集の副作用ではなく、paintmap × triplanar / heightmap-with-noise × triplanar permutation で必発する既存 vulkanize C++ slot allocation bug**

### §5.4 Phase 2d-α verify が前回 (Phase 2c) で triplanar 系を拾わなかった理由 (仮説)

- Phase 2c 末 log では `TerrainMix struct redefinition @ 0:1750` で **F-stage parse が早期 abort**、triplanar permutation の location 衝突は到達せずに表面化していなかった可能性
- Phase 2d-α で TerrainMix + PBRMix guard 適用により F-stage parse が L1750 を通過 → 後続行の `vary_texcoord`/`vary_coords` location 衝突に到達 → **cross-condition cascade reveal**
- これは Phase 2c の Issue B / Issue C cascade reveal と **構造同型 (= 早期 abort 解消による後続 bug 露呈)**、Phase 2d-α 編集自体の副作用ではなく **既存 bug の表面化**

---

## §6 範式継承 + 本 phase 適用範式

### §6.1 継承 (Phase 2c 以前から)

- `feedback_one_step_at_a_time` (1 メッセージ 1 アクション、本 phase は AYA "OK" 単発で進行)
- `feedback_no_scope_shrink` (5 file literal scope 完遂、Fix-2 でも cross-stage 共有 1 file 拡張)
- `feedback_doubt_self_first` (1st verify FAIL 時に「PBRMix guard 漏れ + 設計外 chunk 拡張」を自 commit `0587c574da` の編集起因と疑い、設計 chapter 02 §3.3 表との照合で確定)
- `feedback_render_full_trace_first` (§5 で vulkanize log + GLSL source L238-269 + 編集 diff を 3 軸 trace で「既存 vulkanize bug」と確定、推測で「分からない」と投げない)
- `feedback_no_auto_commit` (AYA "handoff してから commit して" 明示指示後、本 doc 提出 → commit)
- `feedback_self_verify_before_handoff` (本 §5 で triplanar 系 2 件を Phase 2d-α 編集と独立と source-tree trace で self-verify、AYA 動作確認 1 サイクル浪費を防止)
- `feedback_proactive_handoff` (本 doc 自体、AYA 明示指示で起案)
- `feedback_falsification_as_progress` (1st verify FAIL = 範式漏れ + 設計外拡張 2 件を honest に記録 → Fix-2 で η-28-C type 1 再適用)
- `feedback_admit_unknown` (initial 状況で「triplanar 系は既知別件か Phase 2d-α 副作用か」を推測で投げかけたが、AYA 指摘で source-tree trace に切替、§5 で確定)
- η-28-C type 1 (cross-stage same UBO 共有、Phase 2a 由来 + Phase 2c で anonymous-collision check 拡張) — Fix-2 で再適用
- η-28-E 範式 (rename `color`/`size` → `spot_light_color`/`spot_light_size`) — Phase 2d-α 初版で pointLightF / spotLightF に適用
- η-28-F 範式 (struct redefinition guard wrap) — Phase 2d-α 初版で TerrainMix に適用、**本 Fix-1 で PBRMix にも literal 適用範囲拡張**

### §6.2 本 phase 適用 (新規範式 + 拡張)

- **η-28-F 範式の literal 適用範囲完成 (Phase 2d-α Fix-1 で確定)**: 同 file 内に attach 経路で redefinition される struct が **複数存在する場合は全 struct に同型 guard 適用** を範式の literal scope に含める。prep 起草時に **同 file 内 attach 経路 redefinition リスク struct を grep で網羅化**:
  ```bash
  grep -rE "^struct\s+\w+" indra/newview/app_settings/shaders/class*/deferred/pbrterrain*.glsl
  ```
- **η-28-C type 1 範式の anonymous-collision check protocol (Phase 2c から継続 + Phase 2d-α で再実証)**: F stage で UBO 新規追加時、anonymous member 名が **他 set/binding の UBO に既存** = 既存 UBO の cross-stage import (= η-28-C type 1) を default 範式とする。本 phase の Fix-2 で **設計外の独自 chunk 拡張** ではなく既存 block 共有が正しい範式適用と確定。
- **prep 範式追加候補: viewer 起動期待値の vk-α / GL path 分離 (§1.4 で起案)**: 将来 prep doc では **viewer 起動期待値を default GL path (通常表示) と vk-α 経路 (黒画面 + UI)** で分離記述、verify 時の AYA 混乱を防止。

---

## §7 適用ファイル一覧

### §7.1 feat commit `0587c574da` (Phase 2d-α 初版、literal 5 file scope、pivot doc §4.1 由来)

| ファイル | 編集箇所 | UBO/編集 |
|---|---|---|
| `indra/newview/app_settings/shaders/class3/deferred/pointLightF.glsl` | body rename + 自己 PerDrawUBO 削除 | binding 25 維持 (Phase 2c から)、η-28-E 範式 |
| `indra/newview/app_settings/shaders/class3/deferred/spotLightF.glsl` | (a) PerProgramUBO_SpotLightF chunk 3 追加 + (b)(c)(d) | binding 12 (chunk 3 は Fix-2 で撤回)、η-28-E 範式 |
| `indra/newview/app_settings/shaders/class1/deferred/pbrterrainF.glsl` | L47-51 TerrainMix guard wrap | η-28-F 範式 |
| `indra/newview/app_settings/shaders/class1/deferred/pbrterrainUtilF.glsl` | L183-187 TerrainMix guard wrap | η-28-F 範式 |
| `indra/newview/app_settings/shaders/class1/interface/pbrTerrainBakeF.glsl` | L34 TerrainMix guard wrap | η-28-F 範式 future-proofing |

### §7.2 docs commit `9a576884c0` (補助)

| ファイル | 編集箇所 | 内容 |
|---|---|---|
| `docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md` | binding 10 note + η-28-E/F 範式追加 | reference doc 更新 |

### §7.3 feat commit (本 commit、Phase 2d-α Fix、本 handoff 後に提出予定)

| ファイル | 編集箇所 | 修正内容 |
|---|---|---|
| `indra/newview/app_settings/shaders/class1/deferred/pbrterrainF.glsl` | L68-85 | `struct PBRMix` を `#ifndef PBR_MIX_DEFINED ... #endif` で guard wrap (= η-28-F 範式同型適用、初版漏れ補完、Fix-1) |
| `indra/newview/app_settings/shaders/class1/deferred/pbrterrainUtilF.glsl` | L88-105 | 同じ PBRMix guard wrap (Fix-1 連動) |
| `indra/newview/app_settings/shaders/class3/deferred/spotLightF.glsl` | PerProgramUBO_SpotLightF chunk 3 撤回 + MULTI_SPOTLIGHT wrap 書換 | (a) chunk 3 (`vec3 center + float _pad_center`) 削除 = 設計外拡張撤回 (b) MULTI_SPOTLIGHT `#ifdef LL_VULKAN_GLSL` 内で `PerProgramUBO_PointLightV` (set=2 binding=5) を frag stage にも declare + GL path は `uniform vec3 center;` 維持 = η-28-C type 1 範式適用 (Fix-2) |

### §7.4 docs commit (本 handoff 同梱、本 commit 後に提出予定)

| ファイル | 内容 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-complete.md` | 本 doc |

合計: feat 2 commit + docs 2 commit (Phase 2d-α 全体)。

---

## §8 self-verify 結果 (= prep §6 cookbook + Fix 後追加検証)

### §8.1 §6 cookbook 結果 (2 周合算、Fix 適用後)

| grep | 期待 (prep §2.2) | 1st 実測 | 2nd 実測 (Fix 後) | 最終判定 |
|---|---|---|---|---|
| `glslang parse failed` event 数 | 2 | 4 | **3** | △ 期待比 +1 (= 新規 1 件 = §5.2.1 既存 vulkanize bug) |
| `link failed` event 数 | 0 | 1 (Phase 2d-α 起因) | **0 (Phase 2d-α 起因) + 1 (新規、§5.2.2 既存 vulkanize bug)** | **✓ Phase 2d-α 起因 ZERO、残 1 件は独立** |
| 5 program SPIR-V cache miss generated | 5 件 | 4 件 | **5 件 (Deferred SpotLight Shader 含む)** | **✓** |
| viewer 起動 | (GL path 通常表示) | 通常表示 | 通常表示 | **✓** |
| Setting app state to QUITTING | clean | (中断) | clean (L5398) | **✓** |
| Goodbye! | あり | (中断) | あり (L5674) | **✓** |

### §8.2 Phase 2d-α 起因 root 5 件消化確認

| Issue | Phase 2c 末状態 | Phase 2d-α 1st (初版適用後) | Phase 2d-α 2nd (Fix 後) |
|---|---|---|---|
| Issue B (pointLightF `size` undeclared) | 露呈中 | ✓ 消化 (η-28-E 範式) | ✓ 維持 |
| Issue C heightmap permutation (TerrainMix redef) | 露呈中 | ✓ 消化 (η-28-F 範式) | ✓ 維持 |
| Issue C paintmap permutation (TerrainMix redef) | 露呈中 | ✓ 消化 (η-28-F 範式) | ✓ 維持 |
| Issue E (SpotLight `color` undeclared) | 露呈中 | ✓ 消化 (η-28-E 範式) | ✓ 維持 |
| Issue F (SpotLight Anonymous member `center` link) | 未露呈 (Phase 2d-α 初版で reveal) | ✗ 露呈 (初版 chunk 3 拡張起因) | **✓ 消化 (Fix-2 cross-stage 共有適用)** |
| **追加: PBRMix redefinition (Phase 2d-α 初版 reveal)** | 未露呈 (TerrainMix で abort) | ✗ 露呈 (TerrainMix abort 解消で reveal) | **✓ 消化 (Fix-1 guard 適用)** |

5 program SPIR-V 完全 PASS + Phase 2d-α 起因 root 全消化。

### §8.3 §5 既存 vulkanize bug 切り分け self-verify (AYA 「コードみてわからないと言っている?」指摘への source-tree trace)

| 検証項目 | 確認方法 | 結果 |
|---|---|---|
| Phase 2d-α 編集対象 file が `vary_texcoord` / `vary_coords` location 宣言部を編集したか | `git diff 0587c574da^..0587c574da -- pbrterrain*.glsl` + L238-269 範囲確認 | **編集していない** (= Phase 2d-α 編集と独立) |
| `vary_texcoord` location override が vulkanize C++ 側起因か | log の `vulkanizeStageSource: F stage layout in 'vary_texcoord' location override 28 -> 29 (V↔F pair alignment)` 確認 | **C++ 側起因確定** (= llglslshader.cpp:832 でログ出力) |
| triplanar permutation が Phase 2c 末でも同じ問題を持っていたか | Phase 2c 末 log で TerrainMix L1750 abort により F-stage 全 parse 早期停止 = triplanar 系の後続 location 問題は **到達せず未表面化** | **cascade reveal で説明可能** = Phase 2d-α で TerrainMix + PBRMix guard 適用後に F-stage parse が L1750 を通過 → 後続 location 問題に到達 → 既存 bug 露呈 |

→ 「コード見てから」**Phase 2d-α 編集と独立**と確定、推測で「分からない」と投げない。

---

## §9 落穂拾い + Phase 2d-β 起点 + 完了 checklist

### §9.1 本 phase 完了 checklist

- [x] AYA verify-prep handoff レビュー + verify 実施
- [x] feat commit `0587c574da` (Phase 2d-α 初版 5 file) ※前 session で commit 済
- [x] docs commit `9a576884c0` (reference doc 更新) ※前 session で commit 済
- [x] deploy + shader cache clear (1st)
- [x] AYA 1st cold launch + log 採取
- [x] Claude self-verify: PBRMix redef + SpotLight Anonymous member の 2 件 reveal 検出
- [x] AYA 「設計に合わせることはできない?」指摘 → 設計 chapter 02 §3.3 表と照合 → 設計外 chunk 3 拡張を撤回 + cross-stage 共有を Fix-2 として確定
- [x] Fix-1 (PBRMix guard 補完) + Fix-2 (chunk 3 撤回 + cross-stage 共有) を 3 file 編集適用
- [x] deploy + shader cache clear (2nd)
- [x] AYA 2nd cold launch + log 採取
- [x] Claude self-verify: Phase 2d-α 起因 5 root 全消化、残 parse fail 3 + link fail 1 を §5 で trace 確定 (既存 vulkanize bug)
- [x] AYA 「コードみてわからないと言っている?」指摘 → source-tree trace で「Phase 2d-α 編集と独立」と確定 (§5.3)
- [x] AYA 「handoff してから commit して」明示指示
- [x] Phase 2d-α complete handoff 起草 (本 doc)
- [ ] **次 step**: 本 complete handoff doc + Fix commit を提出 (feat commit = §7.3、docs commit = §7.4)
- [ ] AYA push (feature/ayastorm-r41-gl-removal、`feedback_release_flow` で AYA 担当)
- [ ] **次 session**: Phase 2d-β prep 起草 (= 設計 source of truth に従う Phase 0 計測着手の docs 化、`design/09-phase-roadmap.md` §2 起点)

### §9.2 次 session への引き継ぎ事項

1. **Phase 2d-β scope 確定** (= `design/09-phase-roadmap.md` Phase 0 計測着手):
   - 計測対象 = setter 30 entry point + 既存 GL state cookbook 反映
   - 範式継承 = `feedback_design_phase_no_code_write` (`indra/` 改変禁止、Phase 0 計測も docs 化)
   - 着手前提条件 = 致命傷 2 件 + 新規判断 4 件 (Q22-NUM / Q23-K / Q24-S1 / Q25-21CNT) の AYA 判断完了
2. **残課題 (§5.1) の振分** = pbralpha 系 2 件 (Skinned + non-skin V) は **Phase 2d-β / 2d-γ の literal scope 再 frame 候補**、η-27 hidden cascade の系譜継続
3. **既存 vulkanize bug 2 件 (§5.2) の振分** = **Phase 2d-β の Phase 0 計測対象に追加**、vulkanize C++ 側の `vary_texcoord` location override + slot allocation 不整合を計測 + 設計 chapter 群への反映候補化
4. **AYA 判断 4 件 + 修正推奨 20 件** = Phase 2d-α verify 完了とは独立、main session で Phase 2d-β 着手前に AYA に判断仰ぎ
5. **prep doc 範式追加候補 (§1.4 由来)** = 将来 prep doc で「viewer 起動期待値の vk-α / GL path 分離」を明文化

### §9.3 Phase 2c complete §10 表との差分メモ

Phase 2c complete §10 では Phase 2d 候補 = `pbralpha + SpotLight 系 UBO 化 (binding 26+) + Issue B + C struct/alias 修正 (binding 連番未消費)` を予想していた。実際は:
- Phase 2d-α 初版 = Issue B + C + E + F の 4 系統 (binding 連番未消費、η-28-E/F 範式適用)
- pbralpha 系 = **Phase 2d-α では未着手** = Phase 2d-β / 2d-γ 持越 (= η-27 hidden cascade の系譜継続)
- 設計 pivot 後の方針 = Phase 2d-β は **設計 source of truth の Phase 0 計測着手 (docs 化)**、`indra/` 改変は Phase 1+ に持越

---

## §10 reference link

- 前 handoff (η-28 Phase 2c complete): `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2c-complete.md` (commit `5936ee7f2d` 末同梱)
- Phase 2d-α verify prep handoff: `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-verify-prep.md` (本 doc の直前 prep)
- Phase 2d-α feat commit (literal scope): `0587c574da`
- Phase 2d-α docs commit (reference doc): `9a576884c0`
- Phase 2d-α feat commit (Fix、本 commit): `(本 doc 提出後に確定)`
- 設計 chapter 群 (= source of truth): `design/00-charter.md` 〜 `design/10-open-questions.md` + `design/06a-prep.md` + `design/06b.md` + `design/06c.md`
- 設計 chapter 02 §3.3 表 (= SpotLight chunk 構造 source): `design/02-naming-convention.md` §3.3
- audit 集約: `audit-past-b-work-vs-design-2026-06-03.md`
- 第二次査読 report: `design-review-2026-06-03-second-pass.md`
- location/UBO map: `reference-shader-location-map.md` §6-A binding 25 まで反映 + §8 cookbook
- Phase 2d-α 末 cold launch log (2nd、Fix 後): `~/.ayastorm_x64/logs/AYAstorm.log` (parse fail 3 + link fail 1 = 全て Phase 2d-α 編集と独立)
- Phase 2d-α 末 transformed dump 場所: `~/.ayastorm_x64/cache/shader_cache/transformed/` (Phase 2d-β 着手時の source-tree trace 起点候補)

---

**本 complete handoff は η-28 Phase 2d-α 完了状態の source of truth**。Phase 2d-β prep 起草の起点として §9.2 引き継ぎ事項 + §5 既存 vulkanize bug 振分 + AYA 判断 4 件 を参照。
