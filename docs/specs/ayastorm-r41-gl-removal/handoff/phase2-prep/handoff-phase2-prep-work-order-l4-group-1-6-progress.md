# handoff = r41 Phase 2 前提条件 WORK_ORDER.md C-6 §3.5 L4 group 1-6 起案 + 残 group 引継ぎ要約

**起案日**: 2026-06-06
**前提条件 work**: r41 Phase 2 前提条件 = Phase 1 完了後、Phase 2 着手前の設計・工程資料化
**本 session 達成**: §3.5 header + §3.5.1〜§3.5.6 (= 6 group = 25 UBO) 起案完了 + 23 file §12 反映
**次 session 担当**: §3.5.7〜§3.5.16 (= 10 group = 37 UBO) + L4 サマリ + C-8 + C-9 残

---

## §1. 経緯

### §1.1 前 session handoff record

`handoff/phase2-prep/handoff-phase2-prep-work-order-l0-l3-l5-progress.md` (= 前 session 引継ぎ) で C-6 §3.5 L4 62 UBO 16 group 起案開始指示。AYA literal「commit して次 session で C-6 やりましょう」record 2026-06-06。

### §1.2 本 session AYA literal 確定指示 (= 2026-06-06)

1. **§3.5.1 から着手 OK** (= 本 session 開始時確認)
2. **進められるところまで進めてください** (= 自走承認)
3. **基本的に最初に伝えてる条件がクリアされるなら OK** (= 4 原則 + visual regression ゼロ + AYA 既存機能維持遵守前提で推奨案承認)
4. **現状の見た目とほぼ変わらない描画が望まれる** (= visual regression ゼロ §5.4 policy 再確認)
5. **問題発生時の解決は前提条件を破壊しないかぎり推奨案で OK** (= 設計判断 [要 AYA 判断] マークは推奨案を採用、AYA 検証時 reject なら再起案)

---

## §2. 本 session 達成

### §2.1 起案完了 = §3.5 header + 6 group (= 25 UBO)

| # | section | UBO 数 | group 概要 |
|---|---|---|---|
| 0 | §3.5 header | - | L4 全件共通の (4)(7) 共通項 + Layer 定義 + trace 順判定 |
| 1 | §3.5.1 | 3 | aya_sss_skin_flag triple-write (= MaterialUBO_Legacy + PBROpaqueExtraUBO_Legacy + AvatarFParamUBO_Legacy) |
| 2 | §3.5.2 | 7 | aya_visual_realism + chroma_str + light cvar 3 sub-cluster (= AtmoExtra + SkinSSS + PostDeferredF/NoDoFF + PointLightF/SpotLightF/PointLightV) |
| 3 | §3.5.3 | 3 | shadow_target_width triple-write (= ShadowAlphaMaskV + PbrShadowAlphaMaskV + AvatarAlphaShadowV) |
| 4 | §3.5.4 | 2 | box_center/box_size program 識別 dispatch (= OcclusionCubeV + ShadowCubeV) |
| 5 | §3.5.5 | 3 | GLTF texture transform program 識別 dispatch (= PbrOpaqueV + PbrAlphaV + MaterialUBO) |
| 6 | §3.5.6 | 5 | water 系連動 dirty (= WaterFog + WaterV + UnderWaterF + WaterF + WaterHazeV) |

合計起案 25 UBO、~1500 行追加 `design/ubo/WORK_ORDER.md`。

### §2.2 §12 反映済 (= 23 file)

§3.5.1 = 3 file / §3.5.2 = 7 file / §3.5.3 = 3 file / §3.5.4 = 2 file / §3.5.5 = 3 file / §3.5.6 = 5 file = 合計 23 file (= 軽量版 ~15 行 template)。

### §2.3 完了 commit 単位 (= AYA 確認後想定)

| commit | 内容 | 状態 |
|---|---|---|
| C-6-a | §3.5 header + §3.5.1〜§3.5.6 起案 + 23 file §12 反映 | **本 session 完了、AYA review + commit 待ち** |

---

## §3. 残作業 = 次 session 実施

### §3.1 C-6 残: §3.5.7〜§3.5.16 (= 10 group = 37 UBO) 起案

| # | section | UBO 数 | group 概要 | 推定工数 |
|---|---|---|---|---|
| 7 | §3.5.7 | 10 (= 最大) | sky/cloud/atmospheric (= FrameAtmosphere_Lighting + AtmoExtra + Sky V/F + Clouds V/F + Stars F/V + SunDisc + Moon) | L (group 1 件のみで 1 session 消費の可能性大) |
| 8 | §3.5.8 | 5 | velocity (= AvatarSkin + AvatarVelocity + ObjectSkin + SkinnedVelocity + VelocityVParam + VelocityAlphaV) curr/prev pair | L (cadence mismatch 重大 + ring buffer 大物) |
| 9 | §3.5.9 | 5 | reflection probe/IBL (= Global_ReflectionProbes + ReflectionProbe + RadianceGen + IrradianceGen + Gaussian) | M-L (SINGLETON + mip chain) |
| 10 | §3.5.10 | 6 | post-process chain (= Exposure + Luminance + Tonemap + GammaCorrect + ColorGrading + Vignette) | L (AYAstorm r30 Cinematic 13 cvar 連動) |
| 11 | §3.5.11 | 4 | glow chain (= GlowExtract + GlowV + GlowF + GlowCombine) sequential | M (setter 全特定済 8 site) |
| 12 | §3.5.12 | 2 | SMAA (= SMAAParam + SMAABlendWeights) pass chain | S-M |
| 13 | §3.5.13 | 2 | pathfinding (= V + NoNormal) pair | S |
| 14 | §3.5.14 | 2 | GLTF asset (= Asset_GLTFMaterials + Asset_GLTFNodes) pilot 通電済 | M (binding=0/1/2 衝突 verify) |
| 15 | §3.5.15 | 2 | set=2 binding=0 共有 残 (= ClipPlane + LightParams) name-based dispatch | M (pilot zero IS real data 通電済) |
| 16 | §3.5.16 | 1 | PerDrawUBO_MultiLight (LIGHT_COUNT permutation) | S-M |

合計残 37 UBO、推定追加 ~2200 行 (= ~150-200 行/group 平均、§3.5.7 sky/cloud のみ ~400 行)。

**memory `feedback_proactive_handoff` 適用**: 次 session も 1 session で 10 group 完了困難 (= 推定 2-3 session)、各 group 完了時点で commit + handoff 更新推奨。

### §3.2 C-8: §4 4 原則 gate + 09-phase-roadmap.md §2.1 サマリ訂正

**file**: `design/ubo/WORK_ORDER.md` §4 + `design/09-phase-roadmap.md` §2.1 訂正

**§4 起案内容** (= 前 session handoff §3.2 継承):
- §4.1 原則 1: Core プロセス分散実現 (= C1-C6 設計制約 + 各項目 sub-work (7) で評価)
- §4.2 原則 2: 3 OS 共通 (= OS-1〜OS-10 gate + 各項目評価)
- §4.3 原則 3: Phase 2/3 範囲明確 (= Template A R3-R6 所属、O3-2 = r42 移管)
- §4.4 原則 4: OpenGL を殺さない (= dual-path 出荷、`#ifdef LL_VULKAN_GLSL` gate)
- §4.5 visual regression ゼロ (= §5.4 policy、AYA literal 2026-06-06 追加条件)
- §4.6 各項目評価 protocol = sub-work (7) で全 94 項目逐次 check
- §4.7 violation 検知時の対応

**roadmap §2.1 訂正**:
- 前 session revert 済の re-fill (= AYA literal 94 項目維持前提)
- WORK_ORDER.md への双方向 link
- 全体サマリ (= L0-L5 体系 + 件数表)

### §3.3 C-9 残: 各 UBO file §12 Phase 2 sub-work 進捗 反映

**完了済 (= 35 file)**: L1a 3 + L1b 2 + L2 4 + L5 3 + L3 20 + L4 group 1-6 = 23 file = 合計 **35 file**

待ち訂正: 前 session handoff §3.3 で「L3 20 file (= 本 session 起案済、§12 反映のみ残)」と記載されていたが、本 session 内で並走済か confirm 要。本 session 起点では L3 20 file §12 反映済か未確認 (= 前 session 完了 commit f6e56bb7ea 内で実施済の前提)。次 session 開始時に L3 20 file §12 反映状況再確認推奨。

**残 (= ~59 file)**:
- L4 group 7-16 = 37 file (= C-6 残起案完了時に並走)
- L3 20 file (= 前 session 反映済要 confirm)
- 他 (= 確認次第)

### §3.4 commit timing (= AYA 確認後)

- **直近**: 本 handoff + C-6-a 内容 (= §3.5 header + §3.5.1-6 + 23 file §12) を 1 commit にまとめて記録 (= **AYA commit 指示待ち**)
- 次 session 内: §3.5.7〜§3.5.16 各 group 完了時 commit (= 大型 group は途中 handoff 切替)
- 全完了後最終 commit (= C-8 + C-9 残)

---

## §4. 必読 (= 次 session 着手前、memory `feedback_handoff_minimal_pre_req_read` 適用)

**最低限 3 件**:
1. `design/ubo/WORK_ORDER.md` (= **必読**、本 phase 起案 main file、§3.5.7 placeholder + §3.5 全体 + §1.2 sub-work 7 dim template + §3.4 L3 全件 trace 順内位置参照)
2. `design/ubo/READINESS.md` §3 (= C 62 件 16 group 詳細、§3.7-§3.16 = 残 10 group source)
3. 本 handoff doc (= 進捗 + 残作業)

**pinpoint Read (必要時)**:
- `design/ubo/RELATIONS.md` §5 = dirty 連動 group 20+ trigger 整理
- 前 session handoff `handoff-phase2-prep-work-order-l0-l3-l5-progress.md` §1.2 = AYA literal 確定指示 9 件
- 個別 group 着手時に該当 UBO file 5-10 件 batch Read

**memory 必読**:
- `project_r41_phase2_4_principles` (= 4 原則 確定 record)
- `feedback_admit_unknown` (= 推論禁止、不明明示)
- `feedback_design_phase_no_code_write` (= `indra/` 改変ゼロ)
- `feedback_no_scope_shrink` (= AYA literal scope 厳守)
- `feedback_proactive_handoff` (= context 圧迫時自分から引き継ぎ提案)
- `feedback_handoff_minimal_pre_req_read` (= 最低限 3 件 + pinpoint Read)

---

## §5. 起案規律 (= 絶対遵守、本 session 経験継承)

### §5.1 必須遵守

- **1 group ずつ起案 + §12 反映後 AYA review** (= 本 session §3.5.1 で実施、以降 AYA literal「進めてください」で自走、ただし大型 group (= §3.5.7 sky/cloud) は途中 confirm)
- **1 group 内 UBO は batch 5-10 file 並列 Read** (= 1 並列 message で 3-5 file)
- **推論禁止、不明明示** (= memory `feedback_admit_unknown`、[要追加調査] / [要 verify] / [要 AYA 判断] / [要 L0-4 結果反映] マーク付け、本 session 大量に活用)
- **4 原則 死守** + **visual regression ゼロ** (= 全項目 sub-work (6)(7) で評価、AYA literal「現状の見た目とほぼ変わらない描画」継承)
- **命名統一** (= L4-N group/UBO 番号 + AYA literal group 名 + READINESS §3.N mapping 併記)
- **`indra/` 改変ゼロ** (= design-phase 規律)
- **AYA literal scope 厳守** (= AYA literal「推奨案で OK」採用、4 原則 violation 提案禁止)
- **各 UBO 作業完了で §12 反映** (= 軽量版 template、single source of truth = WORK_ORDER.md §3.5.N)

### §5.2 context 容量管理 (= 本 session の教訓)

- **本 session 6 group 起案 + 23 file §12 反映で ~70-80% context 消費 → handoff 切替判断**
- **L4 起案は 1 group ~150-200 行 (= sky/cloud 10 UBO は ~400 行) + 5-7 file §12 反映**
- **大型 group (= §3.5.7 sky/cloud 10 UBO) は 1 session で 1 group のみで終わる可能性大**
- **70-80% 消費で handoff 切替 + 次 session へ**

### §5.3 L4 起案の特異点 (= 本 session 経験継承)

- **cross-UBO 同期 protocol を group 単位で確立** (= 各 group 内 UBO 間 dirty 連動 + data source 共有、RELATIONS.md §5.1 trigger 由来)
- **verify 単位 = group verify** (= group 全件揃って初めて整合 visual、中間状態は暫定 default 値で破綻回避)
- **AYA 既存機能 risk 大 group の機能維持必須** (= §3.5.1/2 = AYAstorm r14-r20 視覚表現章 + r30 BD 改善、§3.5.6 = water + AYA r12.1)
- **L0-4 cadence 再評価結果に依存する group 多数** (= §3.5.1/2/5/6/7/8 全 6 group、PerDraw/PerFrame 降格候補多数)
- **group 内 sub-cluster あり case** (= §3.5.2 = 3 sub-cluster (visual_realism / chroma_str / light cvar) を 1 group 内で起案)
- **group 間 cross-reference 注意** (= §3.5.1 ↔ §3.5.2 SkinSSS / §3.5.3 ↔ §3.5.4 shadow_target / §3.5.5 ↔ §3.5.1 MaterialUBO 排他 / §3.5.5 ↔ §3.5.14 GLTF asset / §3.5.6 ↔ §3.4 L3 waterSign 等)

---

## §A. 本 commit 対象 (= AYA literal 承認後想定)

| file | 種別 | 内容 |
|---|---|---|
| `design/ubo/WORK_ORDER.md` | 訂正 | §3.5 header + §3.5.1〜§3.5.6 起案 6 group 25 UBO (= ~1500 行追加) |
| `design/ubo/MaterialUBO_Legacy.md` | 訂正 | §12 追加 (= L4-1) |
| `design/ubo/PBROpaqueExtraUBO_Legacy.md` | 訂正 | §12 追加 (= L4-1) |
| `design/ubo/AvatarFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-1) |
| `design/ubo/AtmoExtraUBO_Legacy.md` | 訂正 | §12 追加 (= L4-2 sub-cluster (a)) |
| `design/ubo/SkinSSSPrototypeFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-2 sub-cluster (a)) |
| `design/ubo/PerProgramUBO_PostDeferredF.md` | 訂正 | §12 追加 (= L4-2 sub-cluster (b)) |
| `design/ubo/PerProgramUBO_PostDeferredNoDoFF.md` | 訂正 | §12 追加 (= L4-2 sub-cluster (b)) |
| `design/ubo/PerProgramUBO_PointLightF.md` | 訂正 | §12 追加 (= L4-2 sub-cluster (c)) |
| `design/ubo/PerProgramUBO_SpotLightF.md` | 訂正 | §12 追加 (= L4-2 sub-cluster (c)) |
| `design/ubo/PerProgramUBO_PointLightV.md` | 訂正 | §12 追加 (= L4-2 sub-cluster (c)) |
| `design/ubo/PerProgramUBO_ShadowAlphaMaskV.md` | 訂正 | §12 追加 (= L4-3) |
| `design/ubo/PbrShadowAlphaMaskVParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-3) |
| `design/ubo/AvatarAlphaShadowVParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-3) |
| `design/ubo/OcclusionCubeVParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-4) |
| `design/ubo/PerProgramUBO_ShadowCubeV.md` | 訂正 | §12 追加 (= L4-4) |
| `design/ubo/PbrOpaqueVParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-5) |
| `design/ubo/PerProgramUBO_PbrAlphaV.md` | 訂正 | §12 追加 (= L4-5) |
| `design/ubo/MaterialUBO.md` | 訂正 | §12 追加 (= L4-5) |
| `design/ubo/WaterFogUBO_Legacy.md` | 訂正 | §12 追加 (= L4-6) |
| `design/ubo/WaterVParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-6) |
| `design/ubo/UnderWaterFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-6) |
| `design/ubo/PerProgramUBO_WaterF.md` | 訂正 | §12 追加 (= L4-6) |
| `design/ubo/PerProgramUBO_WaterHazeV.md` | 訂正 | §12 追加 (= L4-6) |
| `handoff/phase2-prep/handoff-phase2-prep-work-order-l4-group-1-6-progress.md` | 新規 | 本 handoff |

合計 25 file (= 1 訂正 + 23 §12 反映 + 1 handoff)。
Co-Authored-By 行不在 (= memory `feedback_no_claude_coauthor` 遵守)。
