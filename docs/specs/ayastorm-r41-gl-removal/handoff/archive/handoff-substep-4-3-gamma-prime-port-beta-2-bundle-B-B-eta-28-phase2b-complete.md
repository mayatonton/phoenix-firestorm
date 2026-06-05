# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2b complete handoff

**作成日**: 2026-06-03
**branch**: `feature/ayastorm-r41-gl-removal`
**feat commit**: `1e30d59c4e` (5 shader + reference doc §6-A 4 行追加)
**prep commit**: `c037f6f696` (Phase 2b prep handoff 起草)
**前 phase**: η-28 Phase 2a (`c53f6e0782` + `057cd8b299` + `f8144cce28`、ERROR 26→22 達成)
**状態**: Phase 2b **AYA cold launch verify 完了 + 期待値完全一致達成**。次は Phase 2c prep 起草。

---

## §1 結果サマリー (cold launch 実測 vs prep doc §3 期待)

| 観測項目 | Phase 2a 末 | Phase 2b 末 (実測) | Δ | 期待 | 判定 |
|---|---|---|---|---|---|
| ERROR 行数 | 22 | **18** | -4 | -4 | ✓ 完全一致 |
| glslang parse failed event 数 | 14 | **10** | -4 | -4 | ✓ 完全一致 |
| link failed | 0 | **0** | 0 | 0 | **✓ 10 sub-bundle 連続 ZERO** |
| 4 target program SPIR-V 生成 | (parse fail) | 全成功 | +4 | +4 | ✓ |
| Skinned variant SPIR-V (副次) | 既成功 | 成功維持 | 0 | - | ✓ |
| postDeferredHQDoFF (preventive) | (parse 対象外) | parse 対象外維持 | 0 | 0 | ✓ |
| clean shutdown | 維持 | 維持 | - | - | ✓ |
| cascade reveal | (Phase 2a +2) | **+0** | - | - | ✓ Phase 2a と異なり clean Δ |

**特記**: Phase 2a は Skinned PBR Alpha V cascade + Deferred SpotLight F MULTI cascade で +2 reveal が発生したが、Phase 2b 4 target は **全 root 1 件 / cascade 0 件** の clean profile で、Δ -4 が parse failed event 数と ERROR 行数の両方で揃った。η-28 sub-bundle 内では Phase 2a の reveal が「累積 cascade を引き継いだ」ためで、Phase 2b ではその先に位置するため reveal がなくなった、と読める。

---

## §2 4 target program SPIR-V 生成確認 (cold launch log 実測)

| program 名 (log) | source file | binding | SPIR-V 生成 log L |
|---|---|---|---|
| Godrays Shader | godraysF.glsl + godraysV.glsl | 17 (F-only) | L984 ✓ (2 stages, 2 files) |
| Deferred Post Shader | postDeferredF.glsl + postDeferredNoTCV.glsl | 20 (F-only) | L1291 ✓ |
| AYAstorm Velocity Alpha Shader | velocityAlphaV.glsl + velocityAlphaF.glsl | 19 (V-only) | L1379 ✓ |
| Skinned AYAstorm Velocity Alpha Shader (副次) | skinnedVelocityAlphaV.glsl + velocityAlphaF.glsl | - (skin path) | L1384 ✓ 維持 |
| AYAstorm Volumetric Light Shader | volumetricLightF.glsl (class3) + postDeferredNoTCV.glsl | 18 (F-only) | L1399 ✓ |

**postDeferredHQDoFF preventive 確認**: cold launch ログ全文に "postDeferredHQDoFF" の parse 試行記録なし。cvar `RenderDepthOfFieldHighQuality` default=0 で `mShaderFiles` への push が条件分岐により skip されているため期待通り。AYA が cvar=1 に flip 時、本 phase の予防 wrap が η-28-C cross-variant として効く。

---

## §3 Phase 2b 末 残 ERROR 18 行内訳 (Phase 2c/2d/2e 候補マッピング)

10 root parse failed + 8 cascade 行 = 18 ERROR、parse failed event 数 10 と整合。

| log L | program | stage | error 種別 | root/cascade | 推定 source file | Phase 振分 |
|---|---|---|---|---|---|---|
| L197 | Water Shader | V | non-opaque @ 0:4079 | root | waterV.glsl | **2c** |
| L687 | Skinned Deferred PBR Alpha Shader | F | non-opaque @ 0:3692 | root | pbralphaF.glsl 系 (skin path) | **2d** (pbralpha cascade) |
| L689 | (同上 cascade) | F | non-opaque @ 0:3692 (同 line) | cascade | (同上) | (2d) |
| L690 | (同上 cascade) | F | screen_res redefinition | cascade | (同上) | (2d) |
| L691 | (同上 cascade) | F | missing #endif | cascade | (同上) | (2d) |
| L696 | Deferred PBR Alpha Shader | V | undeclared identifier modelview_projection_matrix @ 0:1252 | root | pbralphaV.glsl | **2d** (pbralpha cascade) |
| L698 | (同上 cascade) | V | undeclared identifier (同 line) | cascade | (同上) | (2d) |
| L699 | (同上 cascade) | V | missing #endif | cascade | (同上) | (2d) |
| L700 | (同上 cascade) | V | compilation terminated | cascade | (同上) | (2d) |
| L710 | Deferred PBR Terrain Shader 0 heightmap-with-noise triplanar | V | non-opaque @ 0:1223 | root | pbrterrainV.glsl | **2c** |
| L712 | (同上、行のみ ERROR ピックアップ) | V | (上記 root の 1 行) | (root) | (同上) | (2c) |
| L717 | Deferred PBR Terrain Shader 0 paintmap triplanar | V | non-opaque @ 0:1115 | root | pbrterrainV.glsl (paintmap variant) | **2c** |
| L719 | (同上 cascade) | V | non-opaque @ 0:1115 (同 line) | cascade | (同上) | (2c) |
| L720 | (同上 cascade) | V | missing #endif | cascade | (同上) | (2c) |
| L746 | Deferred Light Shader | F | non-opaque @ 0:2287 | root | sunLightF.glsl 系 | **2c** |
| L748 | (同上、行のみ ERROR ピックアップ) | F | (上記 root の 1 行) | (root) | (同上) | (2c) |
| L834 | Deferred SpotLight Shader | F | undeclared identifier color @ 0:2628 | root | spotLightF.glsl / lightUtil | **2d** (η-27 cascade) |
| L836 | (同上 cascade) | F | (同 line) | cascade | (同上) | (2d) |
| L837 | (同上 cascade) | F | vector swizzle out of range | cascade | (同上) | (2d) |
| L838 | (同上 cascade) | F | compilation terminated | cascade | (同上) | (2d) |
| L845 | Deferred MultiSpotLight Shader | F | non-opaque @ 0:2385 | root | multiSpotLightF.glsl / lightUtil | **2d** (η-27 cascade) |
| L847 | (同上 cascade) | F | (同 line) | cascade | (同上) | (2d) |
| L848 | (同上 cascade) | F | missing #endif | cascade | (同上) | (2d) |
| L864 | Deferred Blur Light Shader | F | non-opaque @ 0:1730 | root | blurLightF.glsl | **2c** |
| L866 | (同上、行のみ ERROR ピックアップ) | F | (上記 root の 1 行) | (root) | (同上) | (2c) |
| L1294 | Deferred CoF Shader | F | non-opaque @ 0:1721 | root | cofF.glsl | **2c** |
| L1296 | (同上、行のみ ERROR ピックアップ) | F | (上記 root の 1 行) | (root) | (同上) | (2c) |

**注**: ERROR 行 = 18 のうち、複数行が同 root に紐づく場合あり (parse failed event は program 単位、ERROR 行は line 単位)。parse failed event 10 件 vs root 10 件は 1:1 対応。

---

## §4 Phase 2c / 2d scope テーブル (Phase 2b 末 log 実測ベースで再評価)

### §4.1 Phase 2c (literal 6 root、新規 binding 連番 21〜26 候補)

Phase 2a complete handoff §4 で予想していた `cofF + blurLightF + waterF + pbrterrainV` を **Phase 2b 末 log で再確認**、以下に修正:

| 候補 binding | UBO 名 (案) | source .glsl | uniform (cold launch line から推定、Phase 2c prep で確定) | program 名 | 観測 root |
|---|---|---|---|---|---|
| 21 | `PerProgramUBO_CofF` | `class1/deferred/cofF.glsl` | (Phase 2c prep で grep) | Deferred CoF Shader | L1294 |
| 22 | `PerProgramUBO_BlurLightF` | `class1/deferred/blurLightF.glsl` | (Phase 2c prep で grep) | Deferred Blur Light Shader | L864 |
| 23 | `PerProgramUBO_WaterV` (※V) | `class1/deferred/waterV.glsl` (要確認、class2 等 class 上書きあり) | (Phase 2c prep で grep) | Water Shader | L197 |
| 24 | `PerProgramUBO_PbrTerrainV` | `class1/deferred/pbrterrainV.glsl` | (Phase 2c prep で grep、heightmap-with-noise + paintmap permutation 群) | Deferred PBR Terrain Shader 0 ×N permutation | L710 / L717 |
| 25 | `PerProgramUBO_SunLightF` (仮) | `class1/deferred/sunLightF.glsl` 系 (要 class 確認) | (Phase 2c prep で grep) | Deferred Light Shader | L746 |
| (cross-variant) | 23 共有候補 | waterF.glsl / underWater* etc | (要 cross-variant 範式 η-28-C 適用調査) | (program 跨ぎ) | - |

**Phase 2a complete §4 からの修正**:
- `waterF` → 実際は `waterV` (V stage root)、cross-variant で waterF も同 binding 共有要調査
- 新規追加: `sunLightF` (Deferred Light Shader F、L746)
- 想定 4 root → 実測 6 root (waterV + pbrterrainV ×2 permutation + sunLightF + blurLightF + cofF)

### §4.2 Phase 2d (cascade chain 4 root、binding 27+ 候補)

Phase 2c scope と独立した cascade chain 群、η-27 から繰り越し:

| 候補 | UBO 名 (案) | source 概要 | program 名 | 観測 root + cascade |
|---|---|---|---|---|
| 2d-1 | `PerProgramUBO_PbrAlphaF` 系 + cascade | pbralphaF.glsl 系 (Skinned + non-skin) | Skinned Deferred PBR Alpha / Deferred PBR Alpha | L687-691 + L696-700 |
| 2d-2 | `PerProgramUBO_SpotLightF` 既存 (binding 10、η-27 Phase 1d) との関係要再 trace | spotLightF.glsl + lightUtil | Deferred SpotLight Shader | L834-838 |
| 2d-3 | `PerProgramUBO_MultiSpotLightF` (or SpotLightF 共有？) | multiSpotLightF.glsl + lightUtil | Deferred MultiSpotLight Shader | L845-848 |

**特記**: SpotLight 系 root の L834 は `'color' undeclared identifier` で **uniform missing でなく既存宣言の参照解決失敗**。`PerProgramUBO_SpotLightF` (η-27 Phase 1d、binding 10) で wrap 済の uniform にも関わらず F stage で見えていない可能性 → Phase 2d prep で η-27 範式の再 trace が必要 (η-27 hidden cascade と呼んでいた状態が継続)。

### §4.3 Phase 2e (もし necessary、+ 末)

Phase 2c + 2d で 10 root 全消化見込み (ERROR 18 → 0 経路)。残った場合のみ Phase 2e 起草。Phase 2b 末時点では 2e 計画なし。

---

## §5 reference-shader-location-map.md §6-A 更新済 (commit `1e30d59c4e` 同梱)

```markdown
| 2 | 16 | PerProgramUBO_VisualizeBuffersF | F | η-28 Phase 2a |
| 2 | 17 | PerProgramUBO_GodraysF | F | η-28 Phase 2b |
| 2 | 18 | PerProgramUBO_VolumetricLightF | F | η-28 Phase 2b |
| 2 | 19 | PerProgramUBO_VelocityAlphaV | V | η-28 Phase 2b |
| 2 | 20 | PerProgramUBO_PostDeferredF | F (+ HQDoFF cross-variant) | η-28 Phase 2b |
| 2 | 21+ | (空き、η-28 Phase 2c+ 連番継続) | - | - |
```

doc 見出し も `(η-28 Phase 2a 末時点)` → `(η-28 Phase 2b 末時点)` に更新済 (本 commit)。

---

## §6 Phase 2b 適用ファイル一覧 (commit `1e30d59c4e`)

| ファイル | 編集箇所 | UBO |
|---|---|---|
| `indra/newview/app_settings/shaders/class1/deferred/godraysF.glsl` | L114-120 → 4-layer UBO wrap | binding 17 |
| `indra/newview/app_settings/shaders/class3/deferred/volumetricLightF.glsl` | L122-126 → 4-layer UBO wrap | binding 18 |
| `indra/newview/app_settings/shaders/class1/deferred/velocityAlphaV.glsl` | L52 → 4-layer UBO wrap | binding 19 |
| `indra/newview/app_settings/shaders/class1/deferred/postDeferredF.glsl` | L98-101 → 4-layer UBO wrap | binding 20 |
| `indra/newview/app_settings/shaders/class1/deferred/postDeferredHQDoFF.glsl` | L114-116 → 4-layer UBO wrap (同 UBO 名 + binding + guard) | binding 20 (cross-variant) |
| `docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md` | §6-A 表 4 行追加 + 見出し更新 | - |

合計: 6 ファイル変更、84 insertions / 8 deletions。

---

## §7 η-28-C 範式: cross-variant extension の定式化 (Phase 2b 実証 → 範式昇格)

### §7.1 範式記述 (Phase 2a complete §7 の η-28-C 範式 cross-stage 版 + 本 Phase 2b 拡張)

**η-28-C 範式** (cross-stage + cross-variant 統合版):

> 同一 program 内に **複数の uniform 宣言地点** (V/F 両 stage / cvar-selected file variant) が **同名 uniform を共有** する場合、**全宣言地点に同一 UBO 名 + 同一 binding + 同一 `_DEFINED` guard 名** を宣言する。Vulkan は descriptor 1 個共有で扱い、host 側は 1 回 bind で全 stage / variant 参照可能、guard は宣言地点毎の独立 `#define` で衝突なし。

**範式が適用される 2 case**:

1. **cross-stage 共有** (Phase 2a 実証、waterHazeV + waterHazeF):
   - 同一 program の V stage と F stage で同名 uniform を共有
   - 例: `above_water` を waterHazeV (V) + waterHazeF (F) 両者で使用 → V + F 両 file に同 UBO 宣言
2. **cross-variant 共有** (Phase 2b 実証、postDeferredF + postDeferredHQDoFF):
   - 同一 program 内で cvar-selected file variant 切替で異なる .glsl が attach され、両 variant が同名 uniform を共有
   - 例: `res_scale` + `chroma_str` を postDeferredF (cvar=0) + postDeferredHQDoFF (cvar=1) 両者で使用 → 両 file に同 UBO 宣言

### §7.2 cross-variant 検出 protocol (Phase 2c prep 以降の事前検証範式)

1. Phase prep 起草時、source file `mShaderFiles.push_back` 時の cvar 分岐を `llviewershadermgr.cpp` で grep
2. cvar 分岐両側の .glsl で uniform 宣言を grep
3. 同名 uniform を持つ場合は cross-variant 範式適用、両 file に同 UBO 宣言
4. cvar default 値で parse 対象外な側は **preventive wrap** 扱い (Phase 2b waterHazeF / postDeferredHQDoFF と同じ位置付け)

### §7.3 範式範囲外 (本 phase で新規 case として浮上していない)

- **両 stage + 両 variant 同時共有** (V/F × cvar-variant の 4 候補で同名 uniform): 理論上は本 範式の入れ子適用で対応可能 (4 file 全部に同 UBO 宣言)、本 Phase 2b までは未観測
- **program 跨ぎ uniform 共有** (例えば全 atmospherics 系で同名 uniform): set=0 `Frame*` UBO 帯 (binding 0/1/2) で吸収済、本範式の対象外

---

## §8 self-verify 結果 (prep doc §6 cookbook)

### §8.1 §6-A reserved uniform 名整合 (host C++ vs GLSL) - 全 OK

| GLSL 名 | LLShaderMgr enum | host C++ setter | 検証結果 |
|---|---|---|---|
| `aya_r15_godrays_enabled` | AYA_R15_GODRAYS_ENABLED | llsettingsvo.cpp:1001 | ✓ |
| `aya_r15_godrays_phase_exponent` | AYA_R15_GODRAYS_PHASE_EXPONENT | llsettingsvo.cpp:1005 | ✓ |
| `aya_r15_godrays_strength` | AYA_R15_GODRAYS_STRENGTH | llsettingsvo.cpp:1006 | ✓ |
| `godray_res` | GODRAY_RES | pipeline.cpp:5092 | ✓ |
| `godray_multiplier` | GODRAY_MULTIPLIER | pipeline.cpp:5093 | ✓ |
| `falloff_multiplier` | FALLOFF_MULTIPLIER | pipeline.cpp:5094 | ✓ |
| `seconds60` | (未登録) | (なし、BD legacy dead) | ✓ (UBO 含めるが host 連動なし) |
| `last_object_matrix` | LAST_OBJECT_MATRIX (llshadermgr.h:395) | lldrawpool.cpp:845/934 + tree:199 + terrain:245 | ✓ deploy 前 grep 確定 |
| `res_scale` | DOF_RES_SCALE (llshadermgr.h:220) | pipeline.cpp:10023/10051/10083 (gDeferredCoF/Post/DoFCombine) | ✓ deploy 前 grep 確定 |
| `chroma_str` | DEFERRED_CHROMA_STRENGTH (llshadermgr.h:411) | pipeline.cpp:9552/10056/10379 (gDeferredPostNoDoF/Post/PostNoDoFNoise) | ✓ deploy 前 grep 確定 |

**特記**: `chroma_str` は **3 program で host setter 経由** (Post / PostNoDoF / PostNoDoFNoise) → 各 program 独立の `PerProgramUBO_*` で wrap 済 (η-27 Phase 1b PostDeferredNoDoFF binding 12 / η-28 Phase 2b PostDeferredF binding 20)。PostNoDoFNoise は η-29+ Phase で要 trace (現状 plain uniform 残存中の可能性、log 18 ERROR 内には未浮上 = parse 対象外推定)。

### §8.2 §6-B 5 file edit 後の cross-check (re-read 結果)

- 4-layer nesting (`LL_VULKAN_GLSL` / `_DEFINED` guard / `layout` / closing): 5 file 全 ✓
- `};` + `#endif` 対整合: 5 file 全 ✓
- plain uniform 削除漏れなし (`#else` 側 moved): 5 file 全 ✓
- 同 UBO 名 grep で唯一宣言 (postDeferredF + postDeferredHQDoFF は cross-variant 意図共有): ✓

### §8.3 §6-C cold launch verify cookbook 結果

| grep | 期待 | 実測 | 判定 |
|---|---|---|---|
| `^ERROR: 0:` 行数 | 18 | 18 | ✓ |
| `glslang parse failed` event 数 | 10 | 10 | ✓ |
| `link failed\|link error` | 0 | 0 | ✓ |
| `Shutting down` 行存在 | あり | あり (L3612/3635/3641/3683 等) | ✓ |
| 4 target program SPIR-V cache miss generated | 4 件 | 4 件 (L984/1291/1379/1399) | ✓ |
| `RenderDepthOfFieldHighQuality` cvar value | default 0 | (parse 対象外、preventive 効果は flip 時) | ✓ |

### §8.4 §6-D cascade reveal 観察結果

Phase 2b 4 target は **全て root 1 件 / cascade 0 件** の clean profile (Phase 2a Skinned PBR Alpha / SpotLight MULTI のような chain なし)、Δ event 数 = -4 で揃った。**cascade reveal 0 件**、Phase 2a complete §3 の cascade reveal +2 と異なるパターンを今回観測。

---

## §9 範式継承 + 本 phase 適用範式

### 継承 (Phase 2a 以前から)

- `feedback_one_step_at_a_time` (1 メッセージ 1 アクション、本 phase は AYA "OK" 単発で進行)
- `feedback_no_scope_shrink` (5 file literal scope 完遂)
- `feedback_doubt_self_first` (prep §2.4 で literal "postDeferredHQDoFF" が cold launch parse 対象でないと self-grep 発見、cross-variant 範式へ昇華)
- `feedback_render_full_trace_first` (各 uniform を shader tree + host C++ + reserved uniform table の 3 軸 trace)
- `feedback_no_auto_commit` (AYA "OK" 明示後に feat commit)
- `feedback_self_verify_before_handoff` (本 §8 で全項目 self-verify 済)
- `feedback_proactive_handoff` (本 doc 自体)
- η-28-A (dump marker 信用せず source tree grep) — 本 phase 5 file の uniform 確定に適用
- η-28-B (既存 transformed dump で再 cold launch 不要判定) — prep 起草前に活用
- η-28-C (cross-stage same UBO 共有) — Phase 2a 由来

### 本 phase 適用 (新規範式 + 拡張)

- **η-28-C cross-variant 拡張** (本 phase で実証 + 範式昇格): §7 参照
- (新規範式提案なし、η-28-C のスコープ拡大のみ)

---

## §10 落穂拾い + Phase 2c 起点 + 完了 checklist

### 本 phase 完了 checklist

- [x] AYA prep doc レビュー + "OK" 明示
- [x] §4 の 5 file UBO 化 feat commit (`1e30d59c4e`)
- [x] reference-shader-location-map.md §6-A 表に 4 行追加 + 見出し更新 (同 commit 同梱)
- [x] deploy + shader cache clear
- [x] AYA cold launch + log 採取
- [x] Claude self-verify: ERROR 18 / parse failed 10 / link 0 / SPIR-V 4 program 全成功 / clean shutdown ✓
- [x] Phase 2b complete handoff 起草 (本 doc)
- [ ] AYA push (feature/ayastorm-r41-gl-removal、`feedback_release_flow` で AYA 担当)
- [ ] **次 session**: Phase 2c prep 起草 (本 doc §4.1 表 binding 21〜25 起点)
- [ ] **次々 session**: Phase 2c feat 適用 → cold launch verify → complete handoff
- [ ] **次々々 session**: Phase 2d prep 起草 (本 doc §4.2 cascade chain 表起点、η-27 hidden cascade 再 trace 含む)

### 次 session への引き継ぎ事項

1. **Phase 2c prep 起草起点**: 本 doc §4.1 表 (binding 21〜25 候補)
   - cofF / blurLightF / waterV / pbrterrainV (heightmap-with-noise + paintmap variants) / sunLightF (Deferred Light Shader F)
   - waterV は class1 vs class2 等の class 上書きパス調査要 (volumetricLightF と同様、class3 が実 parse 対象だった範例参照)
   - pbrterrainV permutation 群 (heightmap-with-noise triplanar + paintmap triplanar) は同 .glsl の異 permutation の可能性、各 root の transformed dump 確認要
   - cross-variant 候補: waterV / waterF / 各 underwater variant の同名 uniform 調査
2. **Phase 2d prep 起草起点**: 本 doc §4.2 cascade chain 表
   - pbralpha 系 (Skinned + non-skin V/F) は η-27 hidden cascade、Phase 2c で消化されない可能性高い
   - SpotLight / MultiSpotLight の `'color' undeclared identifier` cascade は η-27 Phase 1d binding 10 `PerProgramUBO_SpotLightF` との関係再 trace 必須
3. **dead uniform 清掃**: 本 phase で `seconds60` (BD legacy) を UBO 内含めたまま維持。将来の cleanup phase (Phase 2-?? or η-29 以降) で UBO 削除 + glsl 宣言削除を提案
4. **PostNoDoFNoise の `chroma_str`**: 本 phase scope 外、log 18 ERROR に出ていないため parse 対象外 (cvar default 推定) と判断、Phase 2c 以降で cvar 分岐 + cross-variant 範式適用調査

### Phase 2a complete §4 表との差分メモ

Phase 2a complete §4 では Phase 2c 候補 = `cofF + blurLightF + waterF + pbrterrainV` (4 root) と予想していたが、本 Phase 2b 末実測で:
- `waterF` → 実際は `waterV` (root が V stage)
- 新規追加 `sunLightF` 系 (Deferred Light Shader F、L746)
- pbrterrainV は 2 permutation で各 root → permutation 同 UBO 共有可能性高 (η-28-C cross-variant の permutation 版？)

Phase 2c prep で「permutation 跨ぎ同 UBO 共有」を η-28-C の更なる拡張範式として検討する候補あり (cvar-variant とは異なり、permutation は preprocessor 分岐なので shader 1 file 内で完結する可能性)。

---

## §11 reference link

- 前 handoff (η-28 Phase 2a complete): `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2a-complete.md` (commit `527d572388`)
- Phase 2b prep handoff: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2b-prep.md` (commit `c037f6f696`)
- Phase 2b feat commit: `1e30d59c4e`
- location/UBO map: `docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md` (§6-A binding 20 まで追加済)
- Phase 2b 末 cold launch log: `~/.ayastorm_x64/logs/AYAstorm.log` (3786 行、ERROR 18)
- Phase 2b 末 transformed dump 場所: `~/.ayastorm_x64/cache/shader_cache/transformed/` (Phase 2c prep で η-28-B 範式起点として活用)

---

**本 complete handoff は η-28 Phase 2b 完了状態の source of truth**。Phase 2c prep 起草の起点として §4.1 表 + §10 引き継ぎ事項を参照。
