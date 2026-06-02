# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2a prep handoff

**作成日**: 2026-06-02
**branch**: `feature/ayastorm-r41-gl-removal`
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-27-complete.md`
**状態**: η-28 Phase 2a **着手前 prep** (調査のみ実施、ソース編集ゼロ)。η-27末 cold launch (2026-06-02T13:56:21Z) の transformed dump 442 file を走査、§5 残 16 件 non-opaque uniforms ERROR の **program × dump UUID × stage × source .glsl × uniform 行** を全件同定済。η-28 cold start で再 trace せず即作業選定可能。

---

## §1 本 doc の位置付け

η-27 complete handoff の §5 表は 16 件 ERROR を `(要 transformed dump 確認)` のまま残していた。本 doc はその「dump 同定」step を完了させ、Phase 2a 着手前の **作業選定材料** を提供する。本 session ではソース編集は行わず、handoff doc 起草のみで終了。

η-28 cold start で行うべきこと:
1. 本 doc §3 同定表 + §4 batch 分割案を読了
2. §5 Phase 2a (4 file 集約 UBO 化) 着手 GO 判断
3. (GO の場合) §5 step 1 から literal 実行

---

## §2 調査経路 (本 session で実施した step)

1. **既存 transformed dump 確認**: `~/.ayastorm_x64/cache/shader_cache/transformed/` 配下に η-27末 cold launch 由来 442 file 残存確認 (mtime 2026-06-02 22:56 JST = 13:56 UTC、η-27末 session と一致)。AYA に再 cold launch 依頼不要、既存 dump で source of truth として十分。
2. **log ERROR 文脈 grep**: `~/.ayastorm_x64/logs/AYAstorm.log` の `glslang parse failed for stage type 0x8bXX (program <名>)` + 直下 `ERROR: 0:LINE:` ペアを 16 件全件抽出、program 名と stage type を確定 (0x8b30 = FRAG / 0x8b31 = VERT)。
3. **dump UUID 同定**: 同 log の `dumpTransformedStageSource ... -> <path>/<UUID>_(vert|frag).glsl` 行を grep、16 program 各々の dump UUID 取得。
4. **dump 該当行 read**: 各 dump の ERROR 行 (0:LINE) ± 5 行を Read、`uniform <type> <name>;` 行の正体を確認。
5. **source .glsl 同定**: 各 dump 内の `/** @file <path>.glsl */` marker を grep、ERROR 行直前の最後の @file marker から source .glsl path を確定 (一部 marker が誤記の case (#14 postDeferredVisualizeBuffers) は source tree 側 grep で補正)。

---

## §3 16 件 ERROR → program → source .glsl 完全同定表

| # | log L | 0:src line | program | dump UUID prefix | stage | **source .glsl** | uniform 行 |
|---|---|---|---|---|---|---|---|
| 1 | 201 | 4079 | Water Shader | b029e076 | frag | **waterF.glsl** | `uniform float blend_factor;` |
| 2 | 691 | 3692 | Skinned Deferred PBR Alpha Shader | 3ed016be | frag | **pbralphaF.glsl** | `uniform vec2 screen_res;` (in `#ifdef HAS_SUN_SHADOW`) |
| 3 | 714 | 1223 | Deferred PBR Terrain Shader 0 heightmap-with-noise triplanar | b45ffd11 | vert | **pbrterrainV.glsl** | `uniform vec4[5] terrain_texture_transforms;` |
| 4 | 721 | 1115 | Deferred PBR Terrain Shader 0 paintmap triplanar | 712d8392 | vert | **pbrterrainV.glsl** | `uniform float region_scale;` (in `TERRAIN_PAINT_TYPE == TERRAIN_PAINT_TYPE_PBR_PAINTMAP`) |
| 5 | 750 | 2287 | Deferred Light Shader | ccc8702e | frag | **pointLightF.glsl** | `uniform float sun_wash;` |
| 6 | 849 | 2385 | Deferred MultiSpotLight Shader | 4d9becab | frag | **spotLightF.glsl** | `uniform vec3 center;` (`#if defined(MULTI_SPOTLIGHT)` branch) |
| 7 | 868 | 1730 | Deferred Blur Light Shader | ce47ab74 | frag | **blurLightF.glsl** | `uniform float dist_factor;` (+ blur_size / delta / kern[4] / kern_scale 隣接) |
| 8 | 986 | 2271 | Godrays Shader | 966ce73e | frag | **godraysF.glsl** | `uniform int aya_r15_godrays_enabled;` (+ aya_r15_godrays_phase_exponent / aya_r15_godrays_strength 等の AYAstorm r15 cvar 群) |
| 9 | 999 | 175 | FS Object ID Shader | a6d592cf | frag | **fsObjectIDF.glsl** | `uniform vec4 object_id_packed;` |
| 10 | 1006 | 1124 | Water Haze Shader | e211e9c7 | vert | **waterHazeV.glsl** | `uniform int above_water;` |
| 11 | 1023 | 422 | Deferred Shadow Cube Shader | 5335fee6 | vert | **shadowCubeV.glsl** | `uniform vec3 box_center;` (+ box_size) |
| 12 | 1303 | 1789 | Deferred Post Shader | 0fc22324 | frag | **postDeferredHQDoFF.glsl** | `uniform float res_scale;` (+ chroma_str 隣接、η-27 Phase 1c の postDeferredNoDoFF UBO とは別 file = 集約は別 UBO 要) |
| 13 | 1311 | 1721 | Deferred CoF Shader | 1e990d6a | frag | **cofF.glsl** | `uniform float depth_cutoff;` + norm_cutoff / focal_distance / blur_constant / tan_pixel_angle / magnification 連 6 uniform |
| 14 | 1379 | 180 | Deferred Buffer Visualization Shader | 2049b698 | frag | **postDeferredVisualizeBuffers.glsl** | `uniform float mipLevel;` (dump @file marker は postDeferredNoDoFF と誤記、実 source は VisualizeBuffers — source tree grep で確認済) |
| 15 | 1396 | 398 | AYAstorm Velocity Alpha Shader | c746a642 | vert | **velocityAlphaV.glsl** | `uniform mat4 last_object_matrix;` |
| 16 | 1419 | 2566 | AYAstorm Volumetric Light Shader | 1ecde0e5 | frag | **volumetricLightF.glsl** | `uniform int godray_res;` + godray_multiplier / falloff_multiplier / seconds60 |

### unique source file 集約 = 15 個 (pbrterrainV.glsl が ERROR 3+4 兼ねる)

| file | ERROR 数 | 含まれる program |
|---|---|---|
| waterF.glsl | 1 | Water Shader |
| pbralphaF.glsl | 1 (+ cascade) | Skinned Deferred PBR Alpha Shader / Deferred PBR Alpha Shader |
| pbrterrainV.glsl | 2 | heightmap-with-noise triplanar / paintmap triplanar |
| pointLightF.glsl | 1 | Deferred Light Shader |
| spotLightF.glsl | 1 | Deferred MultiSpotLight Shader (Phase 1 既 UBO の MULTI_SPOTLIGHT 分岐外 member) |
| blurLightF.glsl | 1 | Deferred Blur Light Shader |
| godraysF.glsl | 1 | Godrays Shader |
| fsObjectIDF.glsl | 1 | FS Object ID Shader |
| waterHazeV.glsl | 1 | Water Haze Shader |
| shadowCubeV.glsl | 1 | Deferred Shadow Cube Shader |
| postDeferredHQDoFF.glsl | 1 | Deferred Post Shader |
| cofF.glsl | 1 | Deferred CoF Shader |
| postDeferredVisualizeBuffers.glsl | 1 | Deferred Buffer Visualization Shader |
| velocityAlphaV.glsl | 1 | AYAstorm Velocity Alpha Shader |
| volumetricLightF.glsl | 1 | AYAstorm Volumetric Light Shader |

---

## §4 batch 分割案 (4 batch、η-25/26/27 同形範式継承)

| batch | source files | program 数 | 想定 binding (set=2) | 1 file あたり uniform 規模 | cascade 派生 想定 |
|---|---|---|---|---|---|
| **Phase 2a** | fsObjectIDF / shadowCubeV / waterHazeV / postDeferredVisualizeBuffers | 4 | 13〜16 | 各 1〜2 uniform、最小規模 batch | なし (独立 program、Δ 期待 -4) |
| **Phase 2b** | godraysF / volumetricLightF / velocityAlphaV / postDeferredHQDoFF | 4 | 17〜20 | aya_r15_* / godray_* / velocity 系、各 2〜6 uniform | 軽微 (HQDoFF の chroma_str 隣接以外は独立) |
| **Phase 2c** | cofF / blurLightF / waterF / pbrterrainV | 4 | 21〜24 | depth_cutoff×6 + dist_factor×5 + blend_factor×1 + terrain×2 (合計最大、1 batch で 14 uniform 集約) | 軽微 |
| **Phase 2d** | pbralphaF / pointLightF / spotLightF (MULTI_SPOTLIGHT 既 UBO member 追加検討) | 1〜3 | 25〜 | screen_res cascade (派生 4+6 件と同時消滅予想)、pointLightF は sun_wash 単独、spotLightF MULTI 分岐は Phase 1 既 PerProgramUBO_SpotLightF (10 member, binding=10) の member 追加 or 別 binding 化判断要 | 大 (η-27 Phase 1e-C 同形、cascade 派生 4 missing #endif + 6 redefinition/undeclared/swizzle/terminated と一斉消滅予想) |

### Phase 2a 推奨理由 (作業選定材料)

- **独立 program**: 4 file とも 1 program 専属、他 file の attach 依存なし、η-27 §3 nameless block 事前 trace 範式の適用負荷ゼロ
- **単純 uniform**: 各 1〜2 uniform のみ、集約 UBO の member list が極小、std140 alignment エラーリスク最小
- **cascade 派生なし**: §5 (η-27 complete) の missing #endif 4 件 + その他 6 件はいずれも別 program 由来、Phase 2a 修正で連鎖消滅効果は出ない (代わりに Δ 期待値が clean に -4 で出る = 計測精度高い)
- **safety 最高**: Phase 1 系の前例 (η-27 Phase 1a-d) で同形パターン全成功、新規範式不要
- **Δ 期待**: ERROR 26 → 22 (-4)、派生連鎖消滅は **Phase 2d で一括回収**

### Phase 2b/c/d 着手前 prep (Phase 2a 完了後)

Phase 2b は AYAstorm 独自 r15 cvar 群が含まれるため、godraysF / volumetricLightF の uniform 総数を Phase 2a 完了後に source tree で再 trace 推奨。
Phase 2d は spotLightF の MULTI_SPOTLIGHT 分岐が Phase 1 既 UBO に **member 追加** で済むか、**別 binding** が必要かを cold launch verify で判定する必要あり (compile time `#if defined(MULTI_SPOTLIGHT)` 切り分け対応)。

---

## §5 Phase 2a 着手手順 (η-28 cold start 後の literal 手順)

### step 1: 事前 trace (η-27 §3 範式継承)

各 file の uniform を source tree で完全 read、nameless block / 隣接 uniform / attach 候補確認:

```
indra/newview/app_settings/shaders/class1/deferred/fsObjectIDF.glsl
indra/newview/app_settings/shaders/class1/deferred/shadowCubeV.glsl
indra/newview/app_settings/shaders/class3/deferred/waterHazeV.glsl
indra/newview/app_settings/shaders/class1/deferred/postDeferredVisualizeBuffers.glsl
```

- 各 file の `uniform <type> <name>;` 行を全件列挙
- nameless interface block (`uniform { ... } name;` 形) があるか確認 (η-27 範式: 事前 trace 必須)
- attach 候補 (`#include` / 他 V/F pair) の member 重複可能性確認

### step 2: 集約 UBO 化 (Edit tool で 1 file ずつ)

| file | binding | UBO 名 | member |
|---|---|---|---|
| fsObjectIDF.glsl | set=2, binding=13 | `PerProgramUBO_FsObjectIdF` | `object_id_packed` (vec4) |
| shadowCubeV.glsl | set=2, binding=14 | `PerProgramUBO_ShadowCubeV` | `box_center` (vec3) + `box_size` (vec3) + pad 2 float |
| waterHazeV.glsl | set=2, binding=15 | `PerProgramUBO_WaterHazeV` | `above_water` (int) + pad 3 float |
| postDeferredVisualizeBuffers.glsl | set=2, binding=16 | `PerProgramUBO_VisualizeBuffersF` | `mipLevel` (float) + pad 3 float |

std140 alignment: vec3 + vec3 は 16-byte align で隣り合うと **後者が 16-byte aligned slot に乗らない** ので、shadowCubeV は `vec3 + float pad + vec3 + float pad` の 32 byte 配置で安全。`reference-shader-location-map.md` §6 表に追記候補。

### step 3: 命名 + #define gate

η-3 §3.2 範式継承、`#ifdef LL_VULKAN_GLSL` ガード下に nameless block 宣言、`#else` で従来 `uniform <type> <name>;` 維持 (GL 互換保持)。`#ifndef PER_PROGRAM_UBO_<NAME>_DEFINED` で多重定義防止 (η-3 PerDrawUBO_LightParams パターン)。

### step 4: deploy

```
# shader 配置 (~/ayastorm/app_settings/shaders/ 下に 4 file cp)
cp indra/newview/app_settings/shaders/class1/deferred/fsObjectIDF.glsl \
   ~/ayastorm/app_settings/shaders/class1/deferred/fsObjectIDF.glsl
cp indra/newview/app_settings/shaders/class1/deferred/shadowCubeV.glsl \
   ~/ayastorm/app_settings/shaders/class1/deferred/shadowCubeV.glsl
cp indra/newview/app_settings/shaders/class3/deferred/waterHazeV.glsl \
   ~/ayastorm/app_settings/shaders/class3/deferred/waterHazeV.glsl
cp indra/newview/app_settings/shaders/class1/deferred/postDeferredVisualizeBuffers.glsl \
   ~/ayastorm/app_settings/shaders/class1/deferred/postDeferredVisualizeBuffers.glsl

# shader cache clear (transformed dump 再生成のため必須)
rm -rf ~/.ayastorm_x64/cache/shader_cache/
```

### step 5: AYA cold launch verify 依頼

step 4 完了後 AYA に cold launch 依頼。観測点:
- parse ERROR 数: 26 → **22** (Δ -4 期待)
- 該当 4 file 関連 ERROR (L201 / L999 / L1006 / L1023 / L1379) が消えていること
- link failed = **0** 維持 (8 sub-bundle 連続 ZERO 維持中)
- clean shutdown 維持
- `~/.ayastorm_x64/cache/shader_cache/transformed/` 配下の該当 UUID dump で UBO 化箇所確認可能

### step 6: Δ 表確認 + 派生消滅効果計測

期待 -4 と実測 -4 が一致するか確認 (Phase 2a は cascade 派生想定なし、超過分があれば想定外で要調査)。実測が -3 以下なら uniform 1 件取りこぼし可能性、+0〜+1 件取りこぼしを source line ベースで再特定。

### step 7: AYA 明示 commit 指示後に 1 commit

`feedback_no_auto_commit` 厳守。commit message template:
```
feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2a 適用

- fsObjectIDF / shadowCubeV / waterHazeV / postDeferredVisualizeBuffers の
  4 file を per-program UBO 化、set=2 binding=13〜16 連番割当て
- ERROR 26 → 22 (-4)、link failed = 0 維持 (8 sub-bundle 連続 ZERO)
- η-27 cascade 派生はそのまま (Phase 2d で一括回収予定)
```

### step 8: 後継 handoff doc 起草

Phase 2a 完了 + commit 後、`handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2a-complete.md` 起草。本 doc §4 batch 分割案を更新 (Phase 2b〜2d の binding 連番更新、Phase 2a 実測 Δ 反映)。

---

## §6 範式継承 + 本 session 新規範式

### 継承範式 (η-27 から)

- `feedback_one_step_at_a_time` (verify 1 ステップずつ)
- `feedback_no_scope_shrink` (literal 承認の literal 実行)
- `feedback_doubt_self_first` (cascade 第26層出現時は推論より trace)
- `feedback_render_full_trace_first` (transformed shader cache から source 同定)
- `feedback_no_auto_commit` (AYA 指示までは local edit のみ)
- `feedback_admit_unknown` (Phase 2 残 16 件は推測でなく順次 trace、3 連続外れたら canary 切替)
- `feedback_perf_map_bfs_drill` (層単位で全周回、A=今層全部 / B=1 段深い全部)
- `feedback_self_verify_before_handoff` (AYA に「確認お願いします」する前に全 cvar/uniform/FBO/上流下流 pass を自分で整合確認)
- `feedback_proactive_handoff` (周回境界で能動 handoff、本 doc 起草自体がこれの実行)
- **η-27 新規範式** (本 doc §5 step 1 で継続適用): nameless block UBO 化前に attach 候補の member list 事前 trace / cascade 派生連鎖消滅効果を Δ 期待値に組み込み

### 本 session (η-28 prep) 新規範式

**範式 η-28-A: dump @file marker は誤記の可能性あり、source tree 側 grep でクロスチェック必須**

- 発見: ERROR 14 (postDeferredVisualizeBuffers) の dump 内 @file marker は `postDeferredNoDoFF.glsl` と誤記、実際の source は VisualizeBuffers
- 原因 (推測): LL Vulkan 移行時の dump 機構が @file header を前後 .glsl のコピペで生成 or 一部 program の source 紐付けが dump 段階でずれている
- 対応: dump 内 @file marker を一次 source of truth とせず、`uniform <name>;` を `indra/newview/app_settings/shaders/` 配下 grep で source file をクロスチェック
- η-28+ 適用: source .glsl 同定 step では必ず source tree grep を併用

**範式 η-28-B: 既存 transformed dump で再 cold launch 不要判定可能**

- 発見: η-27末 cold launch (2026-06-02T13:56:21Z) 由来 dump 442 file が `~/.ayastorm_x64/cache/shader_cache/transformed/` に残存、これで 16 件全件同定完了
- 適用条件: 直前 commit 以降に shader 編集が無い場合、既存 dump が source of truth として有効
- η-28+ 適用: handoff 受領後の最初の step で AYA に cold launch 依頼する前に既存 dump の有無 + mtime を確認、十分なら依頼スキップ

---

## §7 UBO binding 占有マップ (η-27末時点 = η-28 着手前)

`reference-shader-location-map.md` §6 表と完全同期 (本 doc 時点で更新なし):

| set | binding | UBO 名 | 起源 sub-step | 状態 |
|---|---|---|---|---|
| 2 | 0 | PerDrawUBO_LightParams | η-3 | 確定 |
| 2 | 1 | PerDrawUBO_MultiLight | η-23 | 確定 |
| 2 | 2 | PerProgramUBO_GammaCorrect | η-24 | 確定 |
| 2 | 3 | PerProgramUBO_AlphaParams | η-25 Phase 1a | 確定 |
| 2 | 4 | PerProgramUBO_ColorGrading | η-25 Phase 1b | 確定 |
| 2 | 5 | PerProgramUBO_PointLightV | η-25 Phase 1c | 確定 |
| 2 | 6 | PerProgramUBO_ShadowAlphaMaskV | η-26 Phase 1a | 確定 |
| 2 | 7 | PerProgramUBO_PostDeferredV | η-26 Phase 1b | 確定 |
| 2 | 8 | PerProgramUBO_FullbrightShinyV | η-26 Phase 1c | 確定 |
| 2 | 9 | PerProgramUBO_FxaaF | η-27 Phase 1a | 確定 |
| 2 | 10 | PerProgramUBO_SpotLightF (10 member) | η-27 Phase 1d + 1e-A | 確定 |
| 2 | 11 | PerProgramUBO_PbrAlphaV | η-27 Phase 1c | 確定 |
| 2 | 12 | PerProgramUBO_PostDeferredNoDoFF | η-27 Phase 1b | 確定 |
| 2 | 13 | (Phase 2a 候補: PerProgramUBO_FsObjectIdF) | η-28 Phase 2a | **着手前 prep** |
| 2 | 14 | (Phase 2a 候補: PerProgramUBO_ShadowCubeV) | η-28 Phase 2a | **着手前 prep** |
| 2 | 15 | (Phase 2a 候補: PerProgramUBO_WaterHazeV) | η-28 Phase 2a | **着手前 prep** |
| 2 | 16 | (Phase 2a 候補: PerProgramUBO_VisualizeBuffersF) | η-28 Phase 2a | **着手前 prep** |
| 2 | 17+ | (空き、Phase 2b 以降で連番継続) | - | - |

---

## §8 落穂拾い / η-28 cold start 前確認 checklist

- [x] η-27末 cold launch 由来 transformed dump (442 file) の存在確認
- [x] 16 件 ERROR ↔ program ↔ dump UUID ↔ stage 同定
- [x] 各 dump の ERROR 行 read による uniform 名確定
- [x] dump @file marker + source tree grep による source .glsl 確定 (16/16)
- [x] batch 分割案 (Phase 2a/2b/2c/2d) 策定
- [x] Phase 2a 着手手順 8 step literal 化
- [x] 範式 η-28-A (dump marker 誤記対応) + η-28-B (既存 dump 流用) 文書化
- [ ] **本 doc commit** (AYA 明示指示後)
- [ ] η-28 cold start 後、§5 step 1 (4 file の事前 trace) を最初に実行
- [ ] Phase 2a 完了後、§4 表に実測 Δ 反映 + 後継 handoff 起草

---

## §9 reference link

- 前 handoff (η-27 complete): `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-27-complete.md`
- location/UBO map: `docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md`
- transformed dump 場所: `~/.ayastorm_x64/cache/shader_cache/transformed/` (η-27末 442 file)
- cold launch log: `~/.ayastorm_x64/logs/AYAstorm.log` (3795 行、session 2026-06-02T13:56:21Z → 13:57:01Z)

---

**本 doc は η-28 Phase 2a 着手用 source of truth**。η-28 cold start 後、本 doc §5 から literal 実行可能。`MEMORY.md` の `project_ayastorm_r41_vulkan_migration.md` から pointer 経由でも到達可能。

η-28 prep 完了 / Phase 2a 着手可能状態。
