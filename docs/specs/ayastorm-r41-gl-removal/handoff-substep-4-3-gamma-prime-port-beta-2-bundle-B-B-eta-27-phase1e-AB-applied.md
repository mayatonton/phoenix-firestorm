# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-27 Phase 1 + Phase 1e A+B 適用済 handoff

**作成日**: 2026-06-02
**branch**: `feature/ayastorm-r41-gl-removal`
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-26-complete.md`
**状態**: Phase 1 (4 file × 10 program UBO 集約) + Phase 1e-A (spotLightF UBO 6 member 除外) + Phase 1e-B (normal_texcoord 20→39 reassign) **deploy 済 + cold launch verify 済**。第25層 cascade として `overlapping location 21` 2 件露呈、Phase 1e-C で対称完封予定。

---

## §1 baseline と Δ (η-26末 → 現在)

| 指標 | η-26末 | Phase 1 deploy | Phase 1e A+B 後 (現在) | 効果 |
|---|---|---|---|---|
| ERROR 件数 | 54 | 44 | **45** | -9 累計 |
| non-opaque uniforms | 24 | 14 | **17** | -7 累計 (Phase 1 wrap) |
| nameless block global scope | 0 | 2 | **0** ✓ | Phase 1e-A 完封 |
| overlapping location 20 | 0 | 2 | **0** ✓ | Phase 1e-B 完封 |
| overlapping location 21 | 0 | 0 | **2** ⚠ | 第25層 cascade 新規露呈 |
| missing #endif (派生) | - | - | 4 (21 派生 2 + non-opaque 派生 2) | - |
| **link failed** | 0 | 0 | **0** ✓ | 7 sub-bundle 連続 ZERO 維持 |
| clean shutdown | ✓ | ✓ | ✓ | 安定 |

cold launch log: `~/.ayastorm_x64/logs/AYAstorm.log` (3795 行、session 開始 2026-06-02T13:48:21Z、shutdown 2026-06-02T13:48:59Z)

---

## §2 Phase 1 適用内容 (4 file × 10 program)

set=2 namespace の binding=9〜12 を新規割当て、η-3 §3.2 PerDrawUBO 派生範式継承。

| binding | UBO 名 | file | 集約 uniforms |
|---|---|---|---|
| 9 | `PerProgramUBO_FxaaF` | `class1/deferred/fxaaF.glsl` L2122-2134 | `rcp_screen_res` (vec2) + `rcp_frame_opt` (vec4) + `rcp_frame_opt2` (vec4) + pad 2 float |
| 10 | `PerProgramUBO_SpotLightF` (Phase 1e-A 後 10 member) | `class3/deferred/spotLightF.glsl` | `proj_near` `proj_ambient_lod` `near_clip` `far_clip` `proj_origin` `sun_wash` `proj_shadow_idx` `shadow_fade` `falloff` `global_light_strength` |
| 11 | `PerProgramUBO_PbrAlphaV` | `class1/deferred/pbralphaV.glsl` L97-L101 | `texture_normal_transform[2]` + `texture_metallic_roughness_transform[2]` |
| 12 | `PerProgramUBO_PostDeferredNoDoFF` | `class1/deferred/postDeferredNoDoFF.glsl` L81-L86 | `chroma_str` (float) + pad 3 float |

broadcast 効果: fxaaF は 4 program 共有 (broadcast effect)、spotLightF/pbralphaV/nodof は各 2 program。

---

## §3 Phase 1e-A: nameless block global scope 衝突解消

### 衝突実体
`spotLightF.glsl` 初回 wrap (`PerProgramUBO_SpotLightF` 17 member) で nameless block 化したが、`deferredUtil.glsl` L98-L108 の `DeferredUtilParamUBO_Legacy` (set=3, binding=6) が同一 program に attach され、6 member (`proj_n / proj_focus / proj_p / proj_lod / proj_range / proj_ambiance`) が **global scope に既に projection 済**だった。

### 解決
`PerProgramUBO_SpotLightF` から該当 6 member を除外、`deferredUtil.glsl` 経由で global scope から参照させる。残 10 member 構成は §2 表通り。

Vulkan 効果: nameless block global scope collision = 2 → **0** ✓

### 教訓 (新規範式候補)
nameless interface block の per-program UBO 化前に、attach 候補 .glsl が宣言する nameless block の member list を **事前 trace 必須**。η-28+ で UBO 化検討時の事前 trace 範式として記録。

---

## §4 Phase 1e-B: overlapping location 20 解消

### 衝突実体
- `normal_texcoord` @ `location=20` (pbralphaV.glsl L149 out / class2/deferred/pbralphaF.glsl L153 in)
- `vary_AdditiveColor` @ `location=20` (atmosphericsVarsV.glsl 由来、`Skinned Deferred PBR Alpha Shader` + `Deferred PBR Alpha Shader` の 2 program に attach merged)

### 解決
V/F pair 同期 reassign: `location=20` → **`location=39`** (§2 で予告済の 39-49 空き帯 candidate を初使用)。

Vulkan 効果: overlapping location 20 = 2 → **0** ✓

### 同期 reassign 必須 (η-25 Phase 1d 範式)
- V: `class1/deferred/pbralphaV.glsl` L149 edit
- F: `class2/deferred/pbralphaF.glsl` L153 edit
- 同 commit に閉じる (mismatch 残置で第N+1層 cascade 発生)

`reference-shader-location-map.md` §3 表に η-27 Phase 1e-B 行追加済、§2 表 39 占有 / 40-49 (10 slot) 空き化済。

---

## §5 残存 第25層 cascade: overlapping location 21

### 衝突実体
- `metallic_roughness_texcoord` @ `location=21` (pbralphaV.glsl L154 out / class2/deferred/pbralphaF.glsl L158 in)
- `vary_AtmosAttenuation` @ `location=21` (atmosphericsVarsV.glsl 由来、attach merged)

被害 program: `Skinned Deferred PBR Alpha Shader` + `Deferred PBR Alpha Shader` (= Phase 1e-B と同じ 2 program、完全対称形)。

η-24 で `vary_tangents[4]` を 21 → 52-55 救出した対称ケース。

### Phase 1e-C 推奨 (literal 次 step)
`metallic_roughness_texcoord` 21 → **40** reassign (V/F 同期):
1. **Edit** `class1/deferred/pbralphaV.glsl` L154 `layout(location=21) out vec2 metallic_roughness_texcoord;` → `layout(location=40) out vec2 metallic_roughness_texcoord;`
2. **Edit** `class2/deferred/pbralphaF.glsl` L158 `layout(location=21) in vec2 metallic_roughness_texcoord;` → `layout(location=40) in vec2 metallic_roughness_texcoord;`
3. **Edit** `docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md`:
   - §3 表 (η-27 Phase 1e-B section) に行追加: `| 40 | vec2 | metallic_roughness_texcoord | 旧 21 | η-27 Phase 1e-C | pbralphaV | pbralphaF |`
   - §2 表 40 行を「(空き)」→「`metallic_roughness_texcoord` (η-27 Phase 1e-C 救出済)」へ更新、41-49 (9 slot) 空き化
4. **Deploy**: `cp pbralphaV + pbralphaF` → `~/ayastorm/app_settings/shaders/...`、`rm -rf ~/.ayastorm_x64/cache/shader_cache/`
5. **AYA cold launch verify 依頼**
6. **Δ 表確認**: overlapping location 21 = 2 → 0、ERROR 45 → 43 期待

### 期待 cascade 連鎖予測
Phase 1e-C 適用後、pbralphaV/F の location 7 (`emissive_texcoord` @ vec2) は vertex attribute namespace 独立で衝突不発予想。ただし `vary_AtmosAttenuation` 系の連鎖が他 location (22 `vary_CloudDensity` 等) で発生する可能性は §2 表通り。

---

## §6 残 17 件 non-opaque uniforms (Phase 2 候補)

cold launch log L196-L1411 から残存 `non-opaque uniforms outside a block` ERROR の出現行 17 件 (source は transformed shader 内行番号、各 ERROR は別 program):

```
L196:  ERROR: 0:4079
L706:  ERROR: 0:1223
L713:  ERROR: 0:1115
L742:  ERROR: 0:2287
L841:  ERROR: 0:2385
L860:  ERROR: 0:1730
L978:  ERROR: 0:2271
L991:  ERROR: 0:175
L998:  ERROR: 0:1124
L1015: ERROR: 0:422
L1295: ERROR: 0:1789
L1303: ERROR: 0:1721
L1371: ERROR: 0:180
L1388: ERROR: 0:398
L1411: ERROR: 0:2566
```

(15 件抜粋。残 2 件は Phase 1e-B/C cascade の派生分含む可能性、Phase 1e-C 適用後に再 count 必要)

η-28 Phase 2 着手時、上記行番号近傍の transformed glsl dump (`~/.ayastorm_x64/cache/shader_cache/transformed/*_vert.glsl` / `*_frag.glsl`) を読んで source .glsl を特定、Phase 1 と同形の per-program UBO 集約を継続。

---

## §7 UBO binding 占有マップ更新 (η-27 末時点 = Phase 1e-C 適用前)

`reference-shader-location-map.md` §6 表は η-27 Phase 1 適用済を反映:

| set | binding | UBO 名 | 起源 sub-step |
|---|---|---|---|
| 2 | 0 | PerDrawUBO_LightParams | η-3 |
| 2 | 1 | PerDrawUBO_MultiLight | η-23 |
| 2 | 2 | PerProgramUBO_GammaCorrect | η-24 |
| 2 | 3 | PerProgramUBO_AlphaParams | η-25 Phase 1a |
| 2 | 4 | PerProgramUBO_ColorGrading | η-25 Phase 1b |
| 2 | 5 | PerProgramUBO_PointLightV | η-25 Phase 1c |
| 2 | 6 | PerProgramUBO_ShadowAlphaMaskV | η-26 Phase 1a |
| 2 | 7 | PerProgramUBO_PostDeferredV | η-26 Phase 1b |
| 2 | 8 | PerProgramUBO_FullbrightShinyV | η-26 Phase 1c |
| 2 | 9 | PerProgramUBO_FxaaF | **η-27 Phase 1a** |
| 2 | 10 | PerProgramUBO_SpotLightF (10 member) | **η-27 Phase 1d + 1e-A** |
| 2 | 11 | PerProgramUBO_PbrAlphaV | **η-27 Phase 1c** |
| 2 | 12 | PerProgramUBO_PostDeferredNoDoFF | **η-27 Phase 1b** |
| 2 | 13+ | (空き、η-28+ 連番継続) | - |

---

## §8 location reassign map (η-27 末時点)

`reference-shader-location-map.md` §3 救出域 (50-60 帯) + 新規 39-49 帯使用:

| location | 救出元 | varying | sub-step |
|---|---|---|---|
| 39 | 20 (vary_AdditiveColor 衝突) | normal_texcoord (pbralphaV/F) | **η-27 Phase 1e-B** |
| 40 | (予定: 21 metallic_roughness_texcoord) | - | **η-27 Phase 1e-C** (未適用) |
| 41-49 | - | (空き) | - |
| 50 | 20 (cascade) | vary_vertex_normal (pbrterrain) | η-24 Phase C |
| 51 | 20 (cascade) | vary_norm (alpha class1/2) | η-24 Phase C |
| 52-55 | 21 (V) / 20 (F) | vary_tangents[4] (pbrterrain) | η-24 case X / η-25 Phase 2 |
| 56-59 | 25 (V) / 24 (F) | vary_signs[4] (pbrterrain) | η-25 Phase 2 |
| 60 | 20 (cascade) | trans_center (pointLight/spotLight) | η-25 Phase 1d |
| 61-63 | - | (空き) | - |

---

## §9 次 session 着手手順 (literal)

1. **branch 確認**: `git status` で `feature/ayastorm-r41-gl-removal` 確認、未 commit edit が 4 file 残ってる (spotLightF / pbralphaV / pbralphaF / fxaaF / postDeferredNoDoFF / reference-shader-location-map.md)
2. **本 doc 読了**: 本 handoff doc を順に読み、§5 Phase 1e-C 推奨を literal 実行
3. **Phase 1e-C 適用**: §5 の 6 step を順次実行 (edit → deploy → verify 依頼 → Δ 表)
4. **AYA cold launch verify** 依頼後の Δ 表確認:
   - 期待: overlapping location 21 = 2 → 0、ERROR 45 → 43、link 段階 ZERO 維持
5. **Phase 1e-C 完封 verify 済 後**: AYA 明示 commit 指示 (`feedback_no_auto_commit`) を待って、η-27 Phase 1 全 sub-phase + 1e-A + 1e-B + 1e-C を 1 commit に閉じる
6. **handoff doc commit**: 本 doc + 後継 `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-27-complete.md` を別 commit で起草・commit

### 範式継承確認
- `feedback_one_step_at_a_time` (verify 1 ステップずつ)
- `feedback_no_scope_shrink` (Phase 1e literal 承認の literal 実行)
- `feedback_doubt_self_first` (cascade 第26層出現時は推論より trace)
- `feedback_render_full_trace_first` (transformed shader cache から source 同定)
- `feedback_no_auto_commit` (AYA 指示までは local edit のみ)
- `feedback_admit_unknown` (Phase 2 残 17 件は推測でなく順次 trace、3 連続外れたら canary 切替)

---

## §10 落穂拾い checklist

- [ ] `metallic_roughness_texcoord` 21 → 40 V/F 同期 edit
- [ ] `reference-shader-location-map.md` §3 表に η-27 Phase 1e-C 行追加
- [ ] `reference-shader-location-map.md` §2 表 40 占有反映 / 41-49 残 9 slot
- [ ] cp 2 file (pbralphaV / pbralphaF) → install dir
- [ ] shader cache 削除
- [ ] AYA cold launch verify 依頼
- [ ] Δ 表確認 (期待: overlapping 21 = 0、ERROR 43)
- [ ] cascade 連鎖の有無 verify (vary_AtmosAttenuation 系 location 22 等)
- [ ] AYA commit 指示後、η-27 Phase 1 全体を 1 commit
- [ ] 後継 handoff doc `*-eta-27-complete.md` 起草

---

**本 handoff doc は次 session 着手用 source of truth**。`MEMORY.md` の `project_ayastorm_r41_vulkan_migration.md` から pointer 経由でも到達可能 (memory は最小 pointer のみ、本 doc が live 詳細)。
