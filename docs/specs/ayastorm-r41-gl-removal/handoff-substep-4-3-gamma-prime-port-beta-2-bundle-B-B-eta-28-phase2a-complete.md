# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2a complete handoff

**作成日**: 2026-06-02
**branch**: `feature/ayastorm-r41-gl-removal`
**commit**: `c53f6e0782` (feat) — Phase 2a 5 file UBO 化
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2a-prep.md`
**状態**: Phase 2a **完了**。ERROR 26 → 22 (Δ -4) prep doc 予想と完全一致、link failed = 0 維持 (9 sub-bundle 連続 ZERO)。

---

## §1 結果サマリ

| 観測項目 | 期待 | 実測 | 判定 |
|---|---|---|---|
| ERROR 行数 | 26 → 22 (Δ -4) | 26 → 22 | ✓ **完全一致** |
| parse failure event 数 | 16 → 12 (Δ -4) | 16 → 14 (Δ -2) | △ +2 cascade reveal |
| link failed | 0 維持 | 0 維持 | ✓ **9 sub-bundle 連続 ZERO** |
| 4 target program SPIR-V 生成 | 全成功 | 全成功 | ✓ |
| waterHazeF cascade error | 出ない (予防修正) | 出ない | ✓ **η-28-C 範式有効** |
| clean shutdown | 維持 | 維持 | ✓ |

### Phase 2a 直接対象 (4 program 全 SPIR-V 生成成功)

| log L | program | UBO 名 (binding) | stage |
|---|---|---|---|
| 995 | FS Object ID Shader | PerProgramUBO_FsObjectIdF (13) | F |
| 1000 | Water Haze Shader | PerProgramUBO_WaterHazeV (15) | **V+F** |
| 1015 | Deferred Shadow Cube Shader | PerProgramUBO_ShadowCubeV (14) | V |
| 1368 | Deferred Buffer Visualization Shader | PerProgramUBO_VisualizeBuffersF (16) | F |

### Phase 2a で実装した 5 file 一覧 (prep doc literal scope 4 + η-28-C 予防修正 1)

| file | UBO | stage 宣言 | 備考 |
|---|---|---|---|
| `class1/deferred/fsObjectIDF.glsl` | PerProgramUBO_FsObjectIdF (13) | F | object_id_packed vec4 単独 |
| `class1/deferred/shadowCubeV.glsl` | PerProgramUBO_ShadowCubeV (14) | V | box_center + box_size (vec3+pad×2) |
| `class3/deferred/waterHazeV.glsl` | PerProgramUBO_WaterHazeV (15) | V | above_water + pad×3 |
| `class3/deferred/waterHazeF.glsl` | PerProgramUBO_WaterHazeV (15) | F (共有宣言) | **η-28-C 予防修正**、同 UBO 名+binding を F stage にも宣言 |
| `class1/deferred/postDeferredVisualizeBuffers.glsl` | PerProgramUBO_VisualizeBuffersF (16) | F | mipLevel + pad×3 |

---

## §2 η-28-C 範式 (新規) 実証

### 発見経緯

η-28 prep §3 では Phase 2a 対象 4 file (waterHazeV.glsl) のみリスト化。Phase 2a step 1 事前 trace 中に `above_water` を全 shader tree で grep → waterHazeF.glsl も同名 uniform を使用していると判明。両者は "Water Haze Shader" program の V/F pair なので、V のみ UBO 化すると η-27 で hidden だった F stage の `uniform int above_water;` (LL_VULKAN_GLSL guard なし) parse error が浮上する見込みだった。

### 対応

waterHazeF.glsl にも同名 UBO ブロック `PerProgramUBO_WaterHazeV` (set=2 binding=15) を宣言。V/F 両 stage で同 block 名+binding を持つ場合、Vulkan 側は **descriptor 1 個を共有**して両 stage が同じ UBO を参照する。host C++ 側からは 1 binding に 1 回 upload するだけで両 stage に反映される。

### 結果

cold launch log L1000 で Water Haze Shader の SPIR-V 生成 OK = V/F 両 stage parse 成功確認。予防修正効果 ✓。

### 範式定式化

> **η-28-C**: program 跨ぎ uniform は V/F 両 stage に同名 UBO ブロック宣言、`set=N binding=M` 共有。host 側は 1 回 bind で両 stage が参照可能。
>
> **適用条件**: 同一 program の V/F pair で同名 uniform を使用、片 stage のみ UBO 化すると他方 stage の plain `uniform` 宣言が Vulkan parse error 浮上の見込み。
>
> **実行 step (Phase 2b+ で踏襲)**: UBO 化対象 file 確定後、各 member uniform 名を `indra/newview/app_settings/shaders/` 全体 grep → 同 program の対 stage file が同名 uniform を使用していれば、両 stage に同 UBO ブロック宣言を入れる。

---

## §3 cascade reveal 観察 (η-27 hidden → η-28 表面化)

prep doc は「Phase 2a 修正で連鎖派生消滅効果は出ない (代わりに Δ 期待値が clean に -4 で出る)」と予想したが、**parse failure event 数**では cascade reveal が 2 件発生。**ERROR 行数**では予想と完全一致 (cascade reveal 件と消滅件が偶然相殺)。

### 新規表面化 2 件 (Phase 2d 対象に追加)

| log L | program | stage | ERROR 種別 | 想定原因 |
|---|---|---|---|---|
| 694 | Deferred PBR Alpha | V | undeclared `modelview_projection_matrix` + missing #endif + compilation terminated | η-27 で Skinned 変種のみ表面化していた pbralphaV.glsl の base 変種 cascade |
| 832 | Deferred SpotLight | F | undeclared `color` + vector swizzle out of range + compilation terminated | η-27 で MULTI 変種のみ表面化していた spotLightF.glsl の MULTI 分岐外 cascade |

両者とも prep doc §4 で **Phase 2d** で「cascade 派生 4+6 件と同時消滅予想」と分類した対象に該当。Phase 2d で pbralphaF (binding 25 候補) / spotLightF MULTI 分岐 (既 PerProgramUBO_SpotLightF binding=10 への member 追加) を実装すれば一括消滅見込み。

### ERROR 種別内訳 (η-28 Phase 2a 末時点 = 全 22 行)

| 種別 | 件数 | 該当 program (例) |
|---|---|---|
| non-opaque uniforms outside a block | 12 | 残 12 program の直接 ERROR |
| missing #endif | 4 | Skinned PBR Alpha / PBR Alpha V / PBR Terrain paintmap / MultiSpotLight |
| undeclared identifier | 2 | PBR Alpha V (modelview_projection_matrix) / SpotLight F (color) |
| compilation terminated | 2 | PBR Alpha V / SpotLight F |
| redefinition | 1 | Skinned PBR Alpha F (screen_res) |
| vector swizzle out of range | 1 | SpotLight F (rgb) |

---

## §4 残 14 program × Phase 2b/c/d 割当て (η-28 prep §4 を実測反映で更新)

### Phase 2b (4 file、binding 17-20 予定): AYAstorm r15 cvar 群 + DoF/velocity

| binding | UBO 名 (候補) | source .glsl | uniform | stage |
|---|---|---|---|---|
| 17 | PerProgramUBO_GodraysF | godraysF.glsl | aya_r15_godrays_enabled + phase_exponent + strength 等 | F (要確認) |
| 18 | PerProgramUBO_VolumetricLightF | volumetricLightF.glsl | godray_res + godray_multiplier + falloff_multiplier + seconds60 | F (要確認) |
| 19 | PerProgramUBO_VelocityAlphaV | velocityAlphaV.glsl | last_object_matrix mat4 | V (要確認) |
| 20 | PerProgramUBO_PostDeferredHQDoFF | postDeferredHQDoFF.glsl | res_scale + chroma_str 隣接 | F (要確認) |

### Phase 2c (4 file、binding 21-24 予定): 最大集約 batch

| binding | UBO 名 (候補) | source .glsl | uniform 数 | stage |
|---|---|---|---|---|
| 21 | PerProgramUBO_CofF | cofF.glsl | depth_cutoff + norm_cutoff + focal_distance + blur_constant + tan_pixel_angle + magnification (6 uniform) | F (要確認) |
| 22 | PerProgramUBO_BlurLightF | blurLightF.glsl | dist_factor + blur_size + delta + kern[4] + kern_scale (5+ uniform) | F (要確認) |
| 23 | PerProgramUBO_WaterF | waterF.glsl | blend_factor 等 | F (要確認、V 対 waterV.glsl との衝突確認必須) |
| 24 | PerProgramUBO_PbrTerrainV | pbrterrainV.glsl | terrain_texture_transforms[5] vec4 + region_scale + ... (heightmap-with-noise / paintmap 両 variant 兼ねる、2 ERROR 同時消滅) | V (要確認) |

### Phase 2d (binding 25+ 予定): pbralpha + pointLight + spotLight cascade 一括回収

| binding | UBO 名 (候補) | source .glsl | uniform | stage |
|---|---|---|---|---|
| 25 | PerProgramUBO_PbrAlphaF | pbralphaF.glsl | screen_res (in HAS_SUN_SHADOW) | F |
| 26 | PerProgramUBO_PointLightF | pointLightF.glsl | sun_wash | F |
| (既 binding=10 への member 追加) | PerProgramUBO_SpotLightF | spotLightF.glsl | center + start + end (MULTI_SPOTLIGHT 分岐) | F |

Phase 2d 完遂で **cascade 派生 4 missing #endif + 6 其他 (= 計 10 件) も一括消滅**見込み。η-27 prep doc の §4 「Phase 2d cascade 一括回収」と一致。

### Phase 2a 完了後の事前 trace 推奨

Phase 2b 着手前に godraysF / volumetricLightF / velocityAlphaV / postDeferredHQDoFF の uniform 全件を `indra/newview/app_settings/shaders/` 全体で grep し、対 stage file (vert 対 frag) の使用クロスチェック必須 (η-28-C 範式)。

特に AYAstorm r15 cvar 群 (godraysF / volumetricLightF) は AYAstorm 独自追加なので、host C++ 側 uniform 設定経路 (cvar binding) との整合も合わせて確認。

---

## §5 reference-shader-location-map.md §6 更新内容 (本 commit 同梱)

`docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md` §6 表に以下を反映:

1. **`stage` 列を新規追加** (V / F / V+F / (per-draw))
2. **既存 13 行に stage を retroactive 記入** (UBO 名 suffix から推定、新規 cascade 表面化時に再確認推奨)
3. **η-28 Phase 2a の 4 行追加** (binding 13-16、特に PerProgramUBO_WaterHazeV は **V+F** で実例化)
4. **stage 列の読み方 + η-28-C 範式の事前 grep ガイダンス**を §6 冒頭に追記
5. **タイトル日付を η-25 末 → η-28 Phase 2a 末**に更新

---

## §6 次 session 計画 (= A-E 資料整備、Phase 2b 着手前提条件)

**AYA 指示 (2026-06-02 session 末)**: Phase 2b 直接着手の前に、Phase 2a で発生した「個別 phase 内で都度発明していた trace / alignment / log 観測手順」を `reference-shader-location-map.md` に集約し、Phase 2b/c/d で共通基盤として再利用可能な資料を作る。

**順序**:
- **次 session** = §6-A の A-E 5 項目を `reference-shader-location-map.md` に追記 (本 phase 2b 着手は行わない)
- **次々 session** = 本 doc §4 Phase 2b 表 (4 file binding 17-20) を起点に Phase 2b 着手

Phase 2b 直前準備 (uniform trace / cvar 経路確認等) は §6-B に移譲。次々 session が読むのは §4 + §6-B + 整備済 reference doc。

---

## §6-A 次 session 作業詳細 (A-E 資料整備、優先順位 + 実装ガイダンス)

本 §6-A は次 session の作業 source of truth。各項目は `docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md` に追記する形で実装。

### A. std140 alignment 既知パターン集 (最優先、Phase 2c で直接必要)

**配置先案**: 新 §4-B (現 §4 Vulkan 制約と GPU 上限の直後)

**含めるべき内容**:
- vec3 単独 → 16 byte (size 12 + 4 pad、明示 pad 推奨)
- vec3 + vec3 → 32 byte (vec3+pad+vec3+pad、shadowCubeV 実例参照)
- int 単独 → 16 byte (size 4 + 12 pad)
- float 単独 → 16 byte (size 4 + 12 pad)
- mat4 → 64 byte (4 vec4)
- vec4[N] → 16N byte
- vec3[N] → 16N byte (各要素は vec4 配置、内部 12 byte data + 4 pad)
- float[N] → 16N byte (各要素は vec4 配置、stride 4 ではなく 16 ⚠️ 罠)
- mixed (vec4 + float) → 16+16 = 32 byte (float 単独 16 align)
- 各 case に「危険な縮め書き例」「安全 explicit pad 書き例」の対比

**Phase 2c で直接必要**: cofF (6 uniform float 群) / pbrterrainV (vec4[5] terrain_texture_transforms) で alignment 設計判断が複雑、本パターン集があれば boilerplate 即流用可能。

### B. set namespace 全体マップ (set=0/1/3 を §6-B として既存 §6 を §6-A に rename)

**配置先案**: 現 §6 を §6-A (set=2 = PerDraw/PerProgram UBO) として保持し、§6-B 以降を新設

**新 §6-B set=0** (per-frame UBO):
- binding=0 FrameViewProj (mat4 群 + screen_res)
- binding=1 FrameLights (sun/moon/light_position[8] 等)
- binding=2+ 既存 / 空き調査要

**新 §6-C set=1** (sampler 系):
- binding=4 diffuseRect
- binding=3 depthMap
- binding=50 exclusionTex (waterHazeF)
- その他 sampler binding の全件棚卸し (Phase 2b/c/d 範囲)

**新 §6-D set=3** (`<Name>UBO_Legacy` 帯):
- binding=9 WaterFogUBO_Legacy
- binding=50 OcclusionCubeVParamUBO_Legacy
- 各 Legacy UBO の存続経緯 (η-X で導入、現在は set=2 へ移行候補か否か)

**用途**: 新 UBO 追加時にどの set に置くか即決可能、命名規則と連動。

### C. UBO 名命名規則 + #define guard 多重定義防止 pattern

**配置先案**: §6 冒頭 (set=2 表より前) の新規ガイダンスセクション

**含めるべき内容**:
- 命名規則:
  - `PerDrawUBO_<Name>` = per-draw 更新頻度 (mTransform / mLight / 描画 call 毎)
  - `PerProgramUBO_<Name><Stage>` = per-program 寿命 (program bind 時に upload)
  - `<Name>UBO_Legacy` = 旧 set=3 帯 (η-6 / η-13 期由来、本 doc §6-D で個別 review 対象)
  - `Frame<Name>` = per-frame (set=0、η-1〜η-5 期 backbone)
- `V/F` suffix は宣言起源 stage を示すだけ、attach 範囲は §6-A stage 列で確認
- `#ifndef PER_PROGRAM_UBO_<NAME>_DEFINED` / `#define ... 1` 多重定義防止 (η-3 PerDrawUBO_LightParams 範式)
- V+F 両 stage 宣言時 (η-28-C 範式) の guard 動作確認パターン (両 stage で別々に `#define` flag が立つので衝突なし)
- 新規 UBO 追加 boilerplate (`#ifdef LL_VULKAN_GLSL` ... `#else` plain uniform ... `#endif` の対称構造)

### D. cascade ERROR 種別表 (log 解析 cookbook)

**配置先案**: 新 §7 (現 §6 の直後)

**含めるべき内容**: ERROR 種別 → 意味 → 対処 → 本 Phase 2a 観測例

| ERROR 文字列 | 意味 | 対処 | Phase 2a 観測例 |
|---|---|---|---|
| `non-opaque uniforms outside a block` | Vulkan で許容されない plain uniform 宣言 | UBO 化必要 (本 doc §3) | L197 waterF blend_factor |
| `missing #endif` | 直前の `#ifdef` 入子内で別 ERROR 発生 → parse 中断、結果として `#endif` 未到達 | **直前 ERROR が root cause**、本 ERROR は表面化現象 | L689 screen_res redefinition cascade |
| `undeclared identifier` | include 経路の uniform/varying 宣言が parse 失敗 で未到達 | 上流 file の parse error 解消、本 ERROR は cascade | L696 modelview_projection_matrix |
| `vector swizzle selection out of range` | `undeclared` 後の continuation (型不明変数を swizzle) | undeclared 解消で連動消滅 | L835 color.rgb |
| `compilation terminated` | parse 中断 sentinel | 直前 ERROR が cascade chain の末端 | L698 / L836 |
| `redefinition` | 同名 uniform が `#ifdef` gate 内外で 2 度宣言 | gate logic 見直し、`#else` 側との二重宣言を排除 | L688 screen_res |

**用途**: log を読むときに「この ERROR は cascade か根本か」即判定可能、Phase 2b/c/d で頻繁に必要。

### E. log 観測 checklist (verify cookbook)

**配置先案**: 新 §8 (現 §7 → §9 に番号繰り下げ)

**含めるべき内容**:

```
# 基本観測 (各 phase 完了後の self-verify cookbook)
grep -c "glslang parse failed"          # parse failure event 数
grep -cE "^ERROR: 0:"                   # ERROR 行数
grep -c "link failed\|link error"       # link 失敗確認 (0 期待)
grep "Shutting down"                    # clean shutdown 確認

# program 別確認 (target 4 program の SPIR-V 生成成功確認)
grep -E "generatePerProgramSPIRV.*for program <NAME>"

# cascade chain 解析
grep -nE "ERROR: 0:[0-9]+:" log.txt     # 全 ERROR を行番号付きで列挙
# 連続行は cascade chain (同一 program の連鎖)、間が空けば別 program

# transformed dump 同定
grep "dumpTransformedStageSource.*<PROGRAM>" log.txt
# → UUID + stage を取得、~/.ayastorm_x64/cache/shader_cache/transformed/<UUID>_<stage>.glsl を read

# fatal 系除外確認
grep -E "VK_ERROR|fatal|panic|abort|signal SIG" log.txt
```

**Phase 完了時 self-verify 手順** (handoff doc 起草前に毎回実施):
1. parse failure event 数の Δ 確認 (期待値と一致するか)
2. ERROR 行数の Δ 確認 (期待値と一致するか)
3. link failed = 0 維持確認
4. target program の SPIR-V 生成成功 grep
5. clean shutdown 維持確認
6. cascade reveal の新規 event 同定 (期待外なら trace)

**用途**: AYA に verify 依頼前に Claude が確認すべき項目を 1 箇所集約 (`feedback_self_verify_before_handoff` の手順化)。

### A-E 実装順序の推奨

1. **C** (命名規則 + #define guard) を先に確立 → 以後の追記が一貫した語彙で書ける
2. **A** (std140 alignment) → Phase 2c 直接必要、設計判断の事典として活用
3. **B** (set namespace 全体) → set=2 以外の binding を確認したい時に参照
4. **D** (cascade ERROR 種別) → Phase 2b 着手後の log 解析で即必要
5. **E** (log 観測 checklist) → 各 phase 完了時の self-verify cookbook

次 session の 1 commit で A-E 全部を `reference-shader-location-map.md` に追記、または C → A の最優先 2 件を先行 commit して残り (B/D/E) は別 commit でも可。AYA 次 session 開始時に判断。

---

## §6-B Phase 2b 直前準備 (次々 session 着手前)

次々 session で Phase 2b 着手前に行うこと (本 doc §4 Phase 2b 表 = 4 file binding 17-20 が起点):

1. Phase 2b 4 file の uniform 全件 source tree 読み込み (η-28-A: dump marker 信用せず source tree grep)
2. 各 uniform 名の **対 stage 使用**確認 (η-28-C、整備済 reference doc §6-A stage 列も参照)
3. AYAstorm r15 cvar 群 (godraysF / volumetricLightF) の host C++ binding 経路確認 (uniform 名 ↔ debug settings)
4. cofF の 6 uniform は std140 alignment が複雑になる可能性、整備済 reference doc §4-B alignment パターン集を参照
5. velocityAlphaV の `mat4 last_object_matrix` は 16 byte * 4 = 64 byte 占有、UBO 単独でも OK

---

## §7 範式継承 + 本 session 新規範式 (Phase 2a 実施で確立)

### 継承範式 (η-27/η-28 prep から)

- `feedback_one_step_at_a_time` (verify 1 ステップずつ)
- `feedback_no_scope_shrink` (literal 承認の literal 実行 + waterHazeF 予防修正は scope 拡張なので OK)
- `feedback_doubt_self_first` (cross-stage trace で自分の prep doc の見落としを早期発見)
- `feedback_render_full_trace_first` (uniform を shader tree 全体 grep)
- `feedback_no_auto_commit` (AYA "OK" 明示後に commit)
- `feedback_admit_unknown` (cascade reveal 2 件は推測でなく log から実測)
- `feedback_perf_map_bfs_drill` (Phase 2a (層 a) 全完遂 → Phase 2b/c/d 1 段深く全周回)
- `feedback_self_verify_before_handoff` (deploy 前に 5 file 自己 re-read + cross-stage grep 完了)
- `feedback_proactive_handoff` (本 doc 自体)
- **η-28-A** (dump marker 信用せず source tree grep でクロスチェック) — Phase 2a の事前 trace で再適用済
- **η-28-B** (既存 transformed dump で再 cold launch 不要判定可能) — Phase 2a 着手判定で適用済 (η-27末 dump 442 file 流用)

### 本 session (η-28 Phase 2a) 新規範式

**範式 η-28-C: program 跨ぎ uniform は V/F 両 stage に同名 UBO 宣言**

- 発見: waterHazeV/F 両 file が `above_water` 使用、V のみ UBO 化では F stage cascade error 浮上の見込み
- 対応: 同 UBO 名 + 同 binding を両 stage に宣言、Vulkan descriptor 1 個共有
- 結果: cold launch verify で SPIR-V 生成成功確認
- η-28+ 適用: Phase 2b/c/d の各 file UBO 化前に member uniform 名 grep でクロスチェック、対 stage 使用判明時は両 stage 同 UBO 宣言

---

## §8 落穂拾い / Phase 2b 着手前 checklist

- [x] Phase 2a 4 file (+ waterHazeF 予防修正 1 file) UBO 化
- [x] deploy + shader cache clear
- [x] AYA cold launch verify (ERROR 26 → 22 / link 0 / SPIR-V 全成功)
- [x] feat commit (`c53f6e0782`)
- [x] reference-shader-location-map.md §6 に stage 列追加
- [x] 本 handoff doc 起草 (§6 = 次 session 計画 / §6-A = A-E 詳細 / §6-B = Phase 2b 直前準備)
- [ ] **docs commit** (本 doc + prep doc + reference doc 更新分、AYA 明示指示後)
- [ ] **次 session**: §6-A A-E 5 項目を `reference-shader-location-map.md` に追記 (Phase 2b 着手前提条件)
- [ ] **次々 session**: 本 doc §4 Phase 2b 表 + §6-B + 整備済 reference doc を起点に Phase 2b 着手

---

## §9 reference link

- 前 handoff (η-28 Phase 2a prep): `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2a-prep.md`
- location/UBO map: `docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md` (本 commit で §6 に stage 列追加)
- Phase 2a feat commit: `c53f6e0782`
- cold launch log: `~/.ayastorm_x64/logs/AYAstorm.log` (3788 行、session 2026-06-02T14:21:00Z → 14:21:39Z、ERROR 22 / link 0 / clean shutdown)
- transformed dump 場所: `~/.ayastorm_x64/cache/shader_cache/transformed/` (η-28 Phase 2a 末 cold launch 由来)

---

**本 doc は η-28 Phase 2a 完了 + 次 session (A-E 資料整備) + 次々 session (Phase 2b 着手) の source of truth**。`MEMORY.md` の `project_ayastorm_r41_vulkan_migration.md` から pointer 経由でも到達可能。

η-28 Phase 2a 完了 / 次 session = A-E 資料整備 / 次々 session = Phase 2b 着手 という 2-session 構成。
