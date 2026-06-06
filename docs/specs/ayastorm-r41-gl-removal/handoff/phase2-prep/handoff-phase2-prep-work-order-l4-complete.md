# handoff = r41 Phase 2 前提条件 WORK_ORDER.md C-6 §3.5 L4 全 16 group 起案完了 + 残 C-8/C-9 引継ぎ

**起案日**: 2026-06-06
**前提条件 work**: r41 Phase 2 前提条件 = Phase 1 完了後、Phase 2 着手前の設計・工程資料化
**本 session 達成**: §3.5.7〜§3.5.16 (= 10 group = 37 UBO) + §3.5.17 L4 サマリ起案完了 + 39 file §12 反映
**次 session 担当**: C-8 (= §4 4 原則 gate + 09-phase-roadmap §2.1 サマリ訂正) + C-9 残 (= L3 20 file §12 反映 confirm + 必要時補完)

---

## §1. 経緯

### §1.1 前 session handoff record

`handoff/phase2-prep/handoff-phase2-prep-work-order-l4-group-1-6-progress.md` (= 前 session 引継ぎ) で §3.5.7〜§3.5.16 (= 10 group = 37 UBO) + L4 サマリ起案開始指示。AYA literal 自走承認継続 2026-06-06「進められるところまで進めてください + 基本的に最初に伝えてる条件がクリアされるなら OK + 推奨案で OK」継承、本 session 開始時 AYA「OK」確認後着手。

### §1.2 本 session AYA literal 確定指示 (= 2026-06-06、前 session 継承)

1. **§3.5.7 から着手 OK** (= 本 session 開始時確認、AYA「OK」)
2. **進められるところまで進めてください** (= 自走承認継続)
3. **基本的に最初に伝えてる条件がクリアされるなら OK** (= 4 原則 + visual regression ゼロ + AYA 既存機能維持遵守前提で推奨案承認)
4. **現状の見た目とほぼ変わらない描画が望まれる** (= visual regression ゼロ §5.4 policy 再確認)
5. **問題発生時の解決は前提条件を破壊しないかぎり推奨案で OK** (= 設計判断 [要 AYA 判断] マークは推奨案を採用、AYA 検証時 reject なら再起案)

---

## §2. 本 session 達成

### §2.1 起案完了 = §3.5.7〜§3.5.16 (= 10 group = 37 UBO) + §3.5.17 L4 サマリ

| # | section | UBO 数 | group 概要 | 工数 |
|---|---|---|---|---|
| 7 | §3.5.7 | 10 | sky/cloud/atmospheric (= 本 phase 最大、5 sub-cluster) | L |
| 8 | §3.5.8 | 6 | velocity curr/prev pair + cadence mismatch + ring buffer 容量 risk | L |
| 9 | §3.5.9 | 5 | reflection probe/IBL (SINGLETON + mip chain) | M-L |
| 10 | §3.5.10 | 6 | post-process chain (AYAstorm r30 Cinematic Control 13 cvar 直結) | L |
| 11 | §3.5.11 | 4 | glow chain sequential (全 4 UBO shell + write 通電済 = 最先進) | M |
| 12 | §3.5.12 | 2 | SMAA pass chain (shared include) | S-M |
| 13 | §3.5.13 | 2 | pathfinding debug pair | S |
| 14 | §3.5.14 | 2 | GLTF asset (= Phase 3 R4 メインターゲット、pilot 通電済) | M |
| 15 | §3.5.15 | 2 | set=2 binding=0 共有残 (= Phase 3 R5 メインターゲット、LightParams pilot zero IS real data 通電済) | M |
| 16 | §3.5.16 | 1 | PerDrawUBO_MultiLight (LIGHT_COUNT permutation 16 件) | S-M |
| 17 | §3.5.17 | - | L4 サマリ + 横断観点 (= 16 group 全件 + unblocking + 共通注記) | S |

合計起案 37 UBO + L4 サマリ = ~2200 行追加 `design/ubo/WORK_ORDER.md`。
累積 §3.5 起案完了 = 16 group 62 UBO + L4 サマリ (= C-6 完了)。

### §2.2 §12 反映済 (= 39 file)

§3.5.7 = 10 file (AtmoExtra 含む cross-group update) / §3.5.8 = 6 file / §3.5.9 = 5 file / §3.5.10 = 6 file / §3.5.11 = 4 file / §3.5.12 = 2 file / §3.5.13 = 2 file / §3.5.14 = 2 file / §3.5.15 = 2 file (1 file = LightParams は既存 pilot 通電 §12 update 含む) / §3.5.16 = 1 file = 合計 **40 entry** (AtmoExtra cross-group ゆえ実 39 file)。

### §2.3 完了 commit 単位 (= AYA 確認後想定)

| commit | 内容 | 状態 |
|---|---|---|
| C-6-a | §3.5 header + §3.5.1〜§3.5.6 起案 + 23 file §12 反映 | **前 commit 0808504ac2 で完了** |
| C-6-b | §3.5.7 sky/cloud 起案 + 10 file §12 反映 | **本 session 完了** |
| C-6-c | §3.5.8 velocity 起案 + 6 file §12 反映 | **本 session 完了** |
| C-6-d | §3.5.9 reflection probe 起案 + 5 file §12 反映 | **本 session 完了** |
| C-6-e | §3.5.10 post-process 起案 + 6 file §12 反映 | **本 session 完了** |
| C-6-f | §3.5.11 glow 起案 + 4 file §12 反映 | **本 session 完了** |
| C-6-g | §3.5.12 SMAA 起案 + 2 file §12 反映 | **本 session 完了** |
| C-6-h | §3.5.13 pathfinding 起案 + 2 file §12 反映 | **本 session 完了** |
| C-6-i | §3.5.14 GLTF asset 起案 + 2 file §12 反映 | **本 session 完了** |
| C-6-j | §3.5.15 set=2 binding=0 共有残 起案 + 2 file §12 反映 | **本 session 完了** |
| C-6-k | §3.5.16 MultiLight + §3.5.17 L4 サマリ起案 + 1 file §12 反映 | **本 session 完了** |

= 本 session 10 commit 単位達成、まとめて 1 commit にすることも可能、AYA judgment。

---

## §3. 残作業 = 次 session 実施

### §3.1 C-8: WORK_ORDER §4 4 原則 gate + 09-phase-roadmap §2.1 サマリ訂正

**file**: `design/ubo/WORK_ORDER.md` §4 (= 新規 section) + `design/09-phase-roadmap.md` §2.1 (= 訂正)

**§4 起案内容**:
- §4.1 原則 1: Core プロセス分散実現 (= C1-C6 設計制約 + 各項目 sub-work (7) で評価)
- §4.2 原則 2: 3 OS 共通 (= OS-1〜OS-10 gate + 各項目評価)
- §4.3 原則 3: Phase 2/3 範囲明確 (= Template A R3-R6 所属、O3-2 = r42 移管)
- §4.4 原則 4: OpenGL を殺さない (= dual-path 出荷、`#ifdef LL_VULKAN_GLSL` gate、ただし host C++ 側は `mUseUBO` runtime flag、memory `project_r41_phase1b_vulkan_host_gate`)
- §4.5 visual regression ゼロ (= §5.4 policy、AYA literal 2026-06-06 追加条件)
- §4.6 各項目評価 protocol = sub-work (7) で全 94 項目逐次 check
- §4.7 violation 検知時の対応

**roadmap §2.1 訂正**:
- 前 session revert 済の re-fill (= AYA literal 94 項目維持前提)
- WORK_ORDER.md への双方向 link
- 全体サマリ (= L0-L5 体系 + 件数表)

**推定工数**: M (= 半日 +、4 原則 gate ~150 行 + roadmap §2.1 ~50 行)

### §3.2 C-9: 各 UBO file §12 Phase 2 sub-work 進捗 反映 confirm

**完了済 (= 累積 ~62 file)**: L1a 3 + L1b 2 + L2 4 + L5 3 + L3 20 + L4 group 1-6 = 23 file (= 前 session 完了) + L4 group 7-16 = 39 file (= 本 session 完了) = 合計 **~62 file** (cross-group AtmoExtra ゆえ実件数微調整要)

**残 confirm 必要**:
- L3 20 file §12 反映 status confirm (= 前 session handoff §3.3 で「§3.3 で L3 20 file §12 反映済」と記載されていたが、前々 commit f6e56bb7ea 内で実施済の前提)、次 session で L3 file 1-2 sample Read で confirm 推奨
- 必要時補完

### §3.3 commit timing (= AYA 確認後)

- **直近**: 本 handoff + C-6-b〜C-6-k 内容 (= §3.5.7-16 + §3.5.17 + 39 file §12) を 1 commit にまとめて記録 (= **AYA commit 指示待ち**)
- 次 session 内: C-8 + C-9 完了後最終 commit

### §3.4 commit 提案 (AYA 判断)

| 案 | 内容 | trade-off |
|---|---|---|
| 案 A | 本 session 全 (= §3.5.7-16 + §3.5.17 + 39 file §12) を 1 commit | 大型だが scope 単一 (= L4 全 group 完了) |
| 案 B | C-6-b〜C-6-k 10 commit に分割 | 細粒度だが事後 review コスト高 |
| 案 C | C-6 全体 (= C-6-a 既 commit + 本 session) を amend で 1 commit に統合 | C-6-a 既 commit 済ゆえ amend 不可 (= 別 commit) |

**推奨**: **案 A** (= 1 commit、scope 単一 = L4 group 7-16 起案完了 + §12 反映、handoff 切替境界明確)

---

## §4. 必読 (= 次 session 着手前、memory `feedback_handoff_minimal_pre_req_read` 適用)

**最低限 3 件**:
1. `design/ubo/WORK_ORDER.md` (= **必読**、本 phase 起案 main file、§4 起案位置 = §3.6 直前、§3.5.17 L4 サマリ末尾 + §4 起案開始)
2. `design/09-phase-roadmap.md` §2.1 (= 訂正対象 section)
3. 本 handoff doc (= 進捗 + 残作業)

**pinpoint Read (必要時)**:
- memory `project_r41_phase2_4_principles` (= §4 起案直接資料、4 原則 確定 record)
- memory `project_ayastorm_r41_design_principles` (= r41 全体 2 大設計原則)
- memory `project_r41_phase1b_vulkan_host_gate` (= §4.4 原則 4 = mUseUBO runtime flag)
- 前 session handoff `handoff-phase2-prep-work-order-l4-group-1-6-progress.md` §1.2 = AYA literal 確定指示 9 件
- L3 file §12 sample Read (= §3.2 confirm 時)

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

- **1 group ずつ起案 + §12 反映後 AYA 自走承認継続** (= 本 session 9 group + L4 サマリ起案 1 session 完走、各 group 完了直後に §12 反映)
- **1 group 内 UBO は batch 3-5 file 並列 Read** (= context 効率優先)
- **推論禁止、不明明示** (= memory `feedback_admit_unknown`、[要追加調査] / [要 verify] / [要 AYA 判断] / [要 L0-4 結果反映] マーク付け、本 session 大量に活用継承)
- **4 原則 死守** + **visual regression ゼロ** (= 全項目 sub-work (6)(7) で評価、AYA literal「現状の見た目とほぼ変わらない描画」継承)
- **命名統一** (= L4-N group/UBO 番号 + AYA literal group 名 + READINESS §3.N mapping 併記)
- **`indra/` 改変ゼロ** (= design-phase 規律)
- **AYA literal scope 厳守** (= AYA literal「推奨案で OK」採用、4 原則 violation 提案禁止)
- **各 UBO 作業完了で §12 反映** (= 軽量版 template、single source of truth = WORK_ORDER.md §3.5.N)

### §5.2 context 容量管理 (= 本 session の教訓)

- **本 session 10 group 起案 + L4 サマリ + 39 file §12 反映で ~85-90% context 消費 → handoff 切替判断**
- **L4 起案は 1 group ~150-300 行 (= sky/cloud 10 UBO は ~400 行) + 5-7 file §12 反映**
- **§3.5.17 L4 サマリは ~100 行**
- **大型 group (= §3.5.7 sky/cloud + §3.5.8 velocity + §3.5.10 post-process) は 1 group ~300 行**
- **小型 group (= §3.5.12 SMAA + §3.5.13 pathfinding + §3.5.16 MultiLight) は 1 group ~150-200 行**
- **70-80% 消費で handoff 切替推奨**

### §5.3 L4 起案規律 (= 本 session 経験継承、絶対遵守)

- cross-UBO 同期 protocol を group 単位で確立 (= RELATIONS.md §5.1 trigger 由来、1 setter call → N UBO 同時 dirty)
- verify 単位 = group verify (= group 全件揃って初めて整合 visual、中間状態は暫定 default 値で破綻回避)
- AYA 既存機能 risk 大 group の機能維持必須:
  - §3.5.10 = AYAstorm r30 Cinematic Control 13 cvar 直結
  - §3.5.7 = AtmoExtra と §3.5.2 SkinSSS 経由交差 (= AYA r14/r16 cvar)
  - §3.5.8 = avatar/object skin curr/prev pair (= memory `project_skin_hash_collision_bom_body`)
  - §3.5.14 = GLTF asset (= Phase 3 R4 メインターゲット)
  - §3.5.15 = LightParams (= Phase 3 R5 メインターゲット、PC-N-13 pilot 通電済)
- L0-4 cadence 再評価結果に依存する group 多数 (= §3.5.7 sky preset / §3.5.8 velocity per-draw / §3.5.9 mip chain loop / §3.5.10 auto-exposure / §3.5.12 SMAA RT metrics)
- group 内 sub-cluster あり case (= §3.5.7 sky/cloud 10 UBO = 5 sub-cluster、§3.5.8 velocity 6 UBO = 3 sub-cluster、§3.5.10 post-process 6 UBO = 4 sub-cluster、§3.5.11 glow 4 UBO = 3 sub-cluster)
- group 間 cross-reference 注意:
  - §3.5.7 ↔ §3.5.2 AtmoExtra (= 9 member 共有、cross-group §12 反映で 1 entry に併記)
  - §3.5.7 ↔ §3.5.14 binding 衝突 3 site (= AtmoExtra binding=0 / SkyV binding=1 / SkyF binding=2 ↔ Asset_GLTFNodes/Materials + Skin_GLTFJoints)
  - §3.5.8 ↔ §3.5.15 set=2 binding=0 共有 6 UBO (= 4 + 2 = 6 件、name-based dispatch 集約)
  - §3.5.9 ↔ §3.5.14 PBR pipeline 統合 (= reflection probe + GLTF asset、Phase 3 R4 連動)
  - §3.5.10 ↔ §3.5.11 ↔ §3.5.12 post-process pipeline sequential dispatch
  - §3.5.15 ↔ §3.5.16 ↔ §3.5.2 sub-cluster (c) light system 統合 (= LightParams + MultiLight + AYAstorm light cvar)

---

## §A. 本 commit 対象 (= AYA literal 承認後想定)

| file | 種別 | 内容 |
|---|---|---|
| `design/ubo/WORK_ORDER.md` | 訂正 | §3.5.7〜§3.5.16 起案 10 group 37 UBO + §3.5.17 L4 サマリ (= ~2200 行追加) |
| `design/ubo/FrameAtmosphere_Lighting.md` | 訂正 | §12 追加 (= L4-7) |
| `design/ubo/SkyVParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-7) |
| `design/ubo/SkyFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-7) |
| `design/ubo/CloudsVParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-7) |
| `design/ubo/CloudsFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-7) |
| `design/ubo/StarsFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-7) |
| `design/ubo/StarsVParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-7) |
| `design/ubo/SunDiscFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-7) |
| `design/ubo/MoonFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-7) |
| `design/ubo/AtmoExtraUBO_Legacy.md` | 訂正 | §12 update (= L4-2 + L4-7 cross-group) |
| `design/ubo/PerDrawUBO_AvatarSkin.md` | 訂正 | §12 追加 (= L4-8) |
| `design/ubo/PerDrawUBO_AvatarVelocity.md` | 訂正 | §12 追加 (= L4-8) |
| `design/ubo/PerDrawUBO_ObjectSkin.md` | 訂正 | §12 追加 (= L4-8) |
| `design/ubo/PerDrawUBO_SkinnedVelocity.md` | 訂正 | §12 追加 (= L4-8) |
| `design/ubo/VelocityVParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-8) |
| `design/ubo/PerProgramUBO_VelocityAlphaV.md` | 訂正 | §12 追加 (= L4-8) |
| `design/ubo/Global_ReflectionProbes.md` | 訂正 | §12 追加 (= L4-9) |
| `design/ubo/ReflectionProbeUBO_Legacy.md` | 訂正 | §12 追加 (= L4-9) |
| `design/ubo/RadianceGenFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-9) |
| `design/ubo/IrradianceGenFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-9) |
| `design/ubo/GaussianFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-9) |
| `design/ubo/ExposureFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-10) |
| `design/ubo/LuminanceFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-10) |
| `design/ubo/TonemapUBO_Legacy.md` | 訂正 | §12 追加 (= L4-10) |
| `design/ubo/PerProgramUBO_GammaCorrect.md` | 訂正 | §12 追加 (= L4-10) |
| `design/ubo/PerProgramUBO_ColorGrading.md` | 訂正 | §12 追加 (= L4-10) |
| `design/ubo/VignetteParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-10) |
| `design/ubo/GlowExtractFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-11) |
| `design/ubo/GlowVParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-11) |
| `design/ubo/GlowFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-11) |
| `design/ubo/GlowCombineFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-11) |
| `design/ubo/SMAAParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-12) |
| `design/ubo/SMAABlendWeightsFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-12) |
| `design/ubo/PathfindingVParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-13) |
| `design/ubo/PathfindingNoNormalVParamUBO_Legacy.md` | 訂正 | §12 追加 (= L4-13) |
| `design/ubo/Asset_GLTFMaterials.md` | 訂正 | §12 追加 (= L4-14) |
| `design/ubo/Asset_GLTFNodes.md` | 訂正 | §12 追加 (= L4-14) |
| `design/ubo/PerDrawUBO_ClipPlane.md` | 訂正 | §12 追加 (= L4-15) |
| `design/ubo/PerDrawUBO_LightParams.md` | 訂正 | §12 追加 (= L4-15) |
| `design/ubo/PerDrawUBO_MultiLight.md` | 訂正 | §12 追加 (= L4-16) |
| `handoff/phase2-prep/handoff-phase2-prep-work-order-l4-complete.md` | 新規 | 本 handoff |

合計 41 file (= 1 訂正 + 39 §12 反映 + 1 handoff)。
Co-Authored-By 行不在 (= memory `feedback_no_claude_coauthor` 遵守)。

---

## §B. 次 session 着手手順

1. 上記必読 3 件 pinpoint Read (= WORK_ORDER.md §4 起案位置 = §3.6 直前 + 09-phase-roadmap.md §2.1 + 本 handoff doc)
2. memory pinpoint 確認 (= project_r41_phase2_4_principles + project_ayastorm_r41_design_principles + project_r41_phase1b_vulkan_host_gate)
3. C-8 §4 起案 (= 7 sub-section §4.1-4.7、推定 ~150 行追加)
4. C-8 09-phase-roadmap.md §2.1 訂正 (= 推定 ~50 行)
5. C-9 L3 file 1-2 sample §12 confirm
6. AYA review → 最終 commit
7. r41 Phase 2 前提条件 完成 → Phase 2 本実装着手判断

**次 session 開始時の最初の AYA 確認**: 「上記要約を確認、本 session で C-8 + C-9 から着手で OK か?」
