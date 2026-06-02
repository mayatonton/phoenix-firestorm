# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-27 complete handoff

**作成日**: 2026-06-02
**branch**: `feature/ayastorm-r41-gl-removal`
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-27-phase1e-AB-applied.md`
**状態**: η-27 全 Phase (1 + 1e-A + 1e-B + 1e-C) **完封 verify 済 + commit 済**。第25層 cascade を含む全 cascade 系 ERROR が完封、η-28 は Phase 2 (残 16 件 non-opaque uniforms) へ移行可能。

---

## §1 η-27 全体 Δ 表 (η-26末 → η-27末)

| 指標 | η-26末 | Phase 1 deploy | Phase 1e A+B | **η-27末 (Phase 1e-C)** | 累計 Δ |
|---|---|---|---|---|---|
| ERROR 件数 (parse `^ERROR: 0:`) | 54 | 44 | 45 | **26** | **-28** |
| `overlapping use of location` 全カテゴリ | 0 | 2 (新規 20) | 2 (新規 21) | **0** ✓ | 完封 |
| `non-opaque uniforms outside a block` | 24 | 14 | 17 | **16** | **-8** |
| `nameless block global scope` collision | 0 | 2 | 0 ✓ | **0** ✓ | 完封 |
| `missing #endif` (cascade 派生) | - | - | 4 | **4** | 維持 |
| その他 cascade 派生 (redefinition / undeclared / swizzle / compilation terminated) | - | - | 多数 | **6** | (Phase 1e-C で連鎖消滅、新分類のみ可視化) |
| **link failed** | 0 | 0 | 0 | **0** ✓ | **8 sub-bundle 連続 ZERO** |
| clean shutdown | ✓ | ✓ | ✓ | ✓ | 維持 |

cold launch log: `~/.ayastorm_x64/logs/AYAstorm.log` (3795 行、session 2026-06-02T13:56:21Z → 13:57:01Z `Vulkan device destroyed` → `Goodbye!` → `status: stopped`)

**η-27 主成果**: parse ERROR 54 → 26 (-52%)、cascade 系 (`overlapping location` / `nameless block`) を完全に 0 へ落とした。残余は Phase 2 着手対象の non-opaque uniforms 16 件 + その派生 missing #endif 4 件 + 派生 6 件。

---

## §2 Phase 1 適用内容 (確定 = 433c204523 commit 済)

set=2 namespace の binding=9〜12 を新規割当て、η-3 §3.2 PerDrawUBO 派生範式継承。

| binding | UBO 名 | file | 集約 uniforms | broadcast 効果 |
|---|---|---|---|---|
| 9 | `PerProgramUBO_FxaaF` | `class1/deferred/fxaaF.glsl` L2122-2134 | `rcp_screen_res` (vec2) + `rcp_frame_opt` (vec4) + `rcp_frame_opt2` (vec4) + pad 2 float | 4 program 共有 |
| 10 | `PerProgramUBO_SpotLightF` (Phase 1e-A 後 10 member) | `class3/deferred/spotLightF.glsl` | `proj_near` `proj_ambient_lod` `near_clip` `far_clip` `proj_origin` `sun_wash` `proj_shadow_idx` `shadow_fade` `falloff` `global_light_strength` | 2 program 共有 |
| 11 | `PerProgramUBO_PbrAlphaV` | `class1/deferred/pbralphaV.glsl` L97-L101 | `texture_normal_transform[2]` + `texture_metallic_roughness_transform[2]` | 2 program 共有 |
| 12 | `PerProgramUBO_PostDeferredNoDoFF` | `class1/deferred/postDeferredNoDoFF.glsl` L81-L86 | `chroma_str` (float) + pad 3 float | 2 program 共有 |

---

## §3 Phase 1e-A 適用内容 (確定 = 433c204523 commit 済)

`PerProgramUBO_SpotLightF` 17 member → 10 member 化。`deferredUtil.glsl` L98-L108 の `DeferredUtilParamUBO_Legacy` (set=3, binding=6) が既に global scope projection 済の 6 member (`proj_n / proj_focus / proj_p / proj_lod / proj_range / proj_ambiance`) を除外、attach merged 後の nameless block global scope collision を解消。

Vulkan 効果: nameless block global scope collision = 2 → **0** ✓

**新規範式採用** (η-28+ 継承): nameless interface block の per-program UBO 化前に、attach 候補 .glsl が宣言する nameless block の member list を **事前 trace 必須**。

---

## §4 Phase 1e-B + 1e-C 適用内容 (確定 = 433c204523 + 4a0d8d7240 commit 済)

pbralpha V/F の vec2 varying 2 件を 20-30 legacy 帯から **39-49 空き帯**へ救出。Phase 1e-B + 1e-C は完全対称形。

| sub-phase | varying | 旧 location | 新 location | 衝突相手 | commit |
|---|---|---|---|---|---|
| Phase 1e-B | `normal_texcoord` | 20 | **39** | atmosphericsVarsV.glsl `vary_AdditiveColor` | 433c204523 |
| Phase 1e-C | `metallic_roughness_texcoord` | 21 | **40** | atmosphericsVarsV.glsl `vary_AtmosAttenuation` | 4a0d8d7240 |

被害 program (共通): `Skinned Deferred PBR Alpha Shader` + `Deferred PBR Alpha Shader` (2 program)。

Vulkan 効果: `overlapping use of location 20` = 2 → 0 ✓、`overlapping use of location 21` = 2 → 0 ✓

### Phase 1e-C で観測した cascade 派生 連鎖消滅効果

Phase 1e A+B 時点で ERROR 45 → Phase 1e-C 後 ERROR 26 (Δ = -19、期待 -2 を大幅超過)。理由:

被害 2 program は location 21 衝突で parse 中断していたため、その上流の `missing #endif` / `redefinition` / `undeclared identifier` / `compilation terminated` 等の派生 ERROR も連鎖して emit していた。Phase 1e-C で parse stage 通過後、これら派生群が一斉に消滅。**cascade 完封 = 単独 ERROR 数以上の連鎖回収**を実証。

η-28+ 教訓: cascade 系 ERROR (`overlapping location` / `nameless block`) は **派生 ERROR を巻き込んで膨れる**。reduce 効果見積もりは Δ - 期待値 で -1〜-2 の超過が予想されるため、Δ 表は「期待 = 完封対象」「実測 = 派生連鎖込み」を別欄で記録すべき。

---

## §5 残 ERROR 内訳 (η-28 着手対象)

cold launch log L201-L1419 から残 26 件 parse ERROR の完全カテゴリ分類:

### 16 件: non-opaque uniforms outside a block (Phase 2 主対象)

各 ERROR は **独立 program × 独立 source line**、Phase 1 と同形の per-program UBO 集約で対応可能。

| # | log line | source line | shader 同定要 |
|---|---|---|---|
| 1 | L201 | 0:4079 | (要 transformed dump 確認) |
| 2 | L691 | 0:3692 | (要 transformed dump 確認、screen_res 系) |
| 3 | L714 | 0:1223 | (要 transformed dump 確認) |
| 4 | L721 | 0:1115 | (要 transformed dump 確認) |
| 5 | L750 | 0:2287 | (要 transformed dump 確認) |
| 6 | L849 | 0:2385 | (要 transformed dump 確認) |
| 7 | L868 | 0:1730 | (要 transformed dump 確認) |
| 8 | L986 | 0:2271 | (要 transformed dump 確認) |
| 9 | L999 | 0:175 | (要 transformed dump 確認) |
| 10 | L1006 | 0:1124 | (要 transformed dump 確認) |
| 11 | L1023 | 0:422 | (要 transformed dump 確認) |
| 12 | L1303 | 0:1789 | (要 transformed dump 確認) |
| 13 | L1311 | 0:1721 | (要 transformed dump 確認) |
| 14 | L1379 | 0:180 | (要 transformed dump 確認) |
| 15 | L1396 | 0:398 | (要 transformed dump 確認) |
| 16 | L1419 | 0:2566 | (要 transformed dump 確認) |

### 4 件: missing #endif (cascade 派生、親 ERROR 修正で連鎖消滅予想)

| # | log line | source line | 親 ERROR |
|---|---|---|---|
| 1 | L693 | 0:3692 | L691 non-opaque + L692 screen_res redefinition |
| 2 | L701 | 0:1252 | L700 modelview_projection_matrix undeclared |
| 3 | L722 | 0:1115 | L721 non-opaque |
| 4 | L850 | 0:2385 | L849 non-opaque |

### 6 件: その他 cascade 派生 (親 ERROR 修正で連鎖消滅予想)

| # | log line | source line | ERROR 種別 |
|---|---|---|---|
| 1 | L692 | 0:3692 | `'screen_res' : redefinition` (non-opaque UBO 化未完で global と nameless block の二重宣言) |
| 2 | L700 | 0:1252 | `'modelview_projection_matrix' : undeclared identifier` |
| 3 | L702 | 0:1252 | `'' : compilation terminated` |
| 4 | L838 | 0:2628 | `'color' : undeclared identifier` |
| 5 | L839 | 0:2628 | `'rgb' : vector swizzle selection out of range` |
| 6 | L840 | 0:2628 | `'' : compilation terminated` |

これら 6 件 + missing #endif 4 件 は親 16 件 non-opaque 系の修正で連鎖消滅する可能性高い (Phase 1e-C で確認した同形パターン)。**Phase 2 着手後の Δ 表で実証必要**。

---

## §6 η-28 着手手順 (literal)

### Phase 2a 着手 (推奨 1 step 目)

1. **transformed shader dump 確認**: cold launch 直前に `rm -rf ~/.ayastorm_x64/cache/shader_cache/` 実施、launch 後 `~/.ayastorm_x64/cache/shader_cache/transformed/` 配下に dump される `*_vert.glsl` / `*_frag.glsl` を grep して §5 表の 16 件各 source line を含む dump file を同定。
2. **source .glsl 同定**: transformed dump の冒頭 `// shader = <name>` コメント (LL 標準) から source `.glsl` file path を確定、`reference-shader-location-map.md` §6 UBO binding 表に追記候補として記録。
3. **batch 分割**: 16 件を file 単位でクラスタリング (1 file が複数 ERROR 持つ場合あり)、1 batch = 3〜4 file × 数 program で集約 UBO 化。η-25 Phase 1a-d / η-26 Phase 1a-c / η-27 Phase 1a-d の前例継承。
4. **集約 UBO 化 1 batch 適用**: set=2 binding=13 から連番継続、PerProgramUBO_*N* 命名 (η-3 §3.2 範式)。
5. **deploy → cold launch verify → Δ 表確認**: ERROR 16 → 12 etc 段階的減算、cascade 派生 (missing #endif / redefinition 等) の連鎖消滅も同時観測。
6. **AYA 明示 commit 指示後** に 1 commit。`feedback_no_auto_commit` 厳守。

### 範式継承確認 (η-28 共通)

- `feedback_one_step_at_a_time` (verify 1 ステップずつ)
- `feedback_no_scope_shrink` (literal 承認の literal 実行)
- `feedback_doubt_self_first` (cascade 第26層出現時は推論より trace)
- `feedback_render_full_trace_first` (transformed shader cache から source 同定)
- `feedback_no_auto_commit` (AYA 指示までは local edit のみ)
- `feedback_admit_unknown` (Phase 2 残 16 件は推測でなく順次 trace、3 連続外れたら canary 切替)
- `feedback_perf_map_bfs_drill` (層単位で全周回、A=今層全部 / B=1 段深い全部)
- **η-27 新規範式** (§3 §4 §6 で確立): nameless block UBO 化前に attach 候補の member list 事前 trace / cascade 派生連鎖消滅効果を Δ 期待値に組み込み

---

## §7 UBO binding 占有マップ (η-27末時点 確定)

`reference-shader-location-map.md` §6 表と完全同期:

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
| 2 | 9 | PerProgramUBO_FxaaF | η-27 Phase 1a |
| 2 | 10 | PerProgramUBO_SpotLightF (10 member) | η-27 Phase 1d + 1e-A |
| 2 | 11 | PerProgramUBO_PbrAlphaV | η-27 Phase 1c |
| 2 | 12 | PerProgramUBO_PostDeferredNoDoFF | η-27 Phase 1b |
| 2 | 13+ | (空き、η-28+ 連番継続) | - |

---

## §8 location reassign map (η-27末時点 確定)

`reference-shader-location-map.md` §3 表と完全同期:

| location | type | varying | 救出元 | 救出 sub-step | V file | F file |
|---|---|---|---|---|---|---|
| 39 | vec2 | `normal_texcoord` | 旧 20 (vary_AdditiveColor 衝突) | η-27 Phase 1e-B | pbralphaV | pbralphaF |
| 40 | vec2 | `metallic_roughness_texcoord` | 旧 21 (vary_AtmosAttenuation 衝突) | η-27 Phase 1e-C | pbralphaV | pbralphaF |
| 41-49 | - | (空き、η-28+ candidate) | - | - | - | - |
| 50 | vec3 | `vary_vertex_normal` | 旧 20 | η-24 Phase C | pbrterrainV | pbrterrainUtilF |
| 51 | vec3 | `vary_norm` | 旧 20 | η-24 Phase C | alphaV (class1/2) | alphaF (class1/2) |
| 52,53,54,55 | vec3[4] | `vary_tangents[4]` | 旧 21 (V) / 旧 20 (F) | η-24 case X / η-25 Phase 2 | pbrterrainV | pbrterrainF |
| 56,57,58,59 | float[4] | `vary_signs[4]` | 旧 25 (V) / 旧 24 (F) | η-25 Phase 2 | pbrterrainV | pbrterrainF |
| 60 | vec3 | `trans_center` | 旧 20 (V+F+F) | η-25 Phase 1d | pointLightV | pointLightF / spotLightF |
| 61-63 | - | (空き、最大 64 component 上限まで残 4 slot) | - | - | - | - |

---

## §9 η-27 commit 履歴

```
4a0d8d7240 feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-27 Phase 1e-C 適用
47ce25e7bb docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-27 Phase 1+1e A+B handoff 起草
433c204523 feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-27 Phase 1 + 1e-A + 1e-B 適用
```

η-26 から積み増し 3 commit (Phase 1 + 1e-A + 1e-B が 1 commit、Phase 1e-C が 1 commit、handoff doc 2 件で計 +1)。

---

## §10 落穂拾い / 残作業 checklist (η-28 着手前確認)

- [x] Phase 1e-C V/F 同期 reassign 適用
- [x] reference-shader-location-map.md §2 §3 表更新
- [x] cold launch verify (`overlapping location 21` = 0 / link ZERO)
- [x] Phase 1e-C 単独 commit (4a0d8d7240)
- [x] 本 complete handoff doc 起草
- [ ] 本 doc commit (本作業ステップ)
- [ ] η-28 着手時に §6 step 1 (transformed shader dump 同定) を最初に実行
- [ ] η-28 Phase 2a 適用後、§5 派生 4+6 件の連鎖消滅実証
- [ ] location 41-49 (9 slot) と 61-63 (3 slot) で残 12 slot を η-28+ で消費予定

---

**本 doc は η-28 着手用 source of truth**。`MEMORY.md` の `project_ayastorm_r41_vulkan_migration.md` から pointer 経由でも到達可能 (memory は最小 pointer のみ、本 doc が live 詳細)。

η-27 完了 / η-28 移行可能状態。
