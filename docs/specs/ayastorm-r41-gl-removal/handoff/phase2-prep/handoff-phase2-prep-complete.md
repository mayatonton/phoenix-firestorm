# handoff = r41 Phase 2 前提条件 work 完走 = C-1〜C-9 全完了 + Phase 2 本実装着手判断 引継ぎ

**起案日**: 2026-06-06
**前提条件 work**: r41 Phase 2 前提条件 = Phase 1 完了後、Phase 2 着手前の設計・工程資料化 work
**本 session 達成**: C-8 (= WORK_ORDER §4 4 原則 gate 起案 + 09-phase-roadmap §2.1.1 サマリ訂正) + C-9 (= L3 20 file §12 反映)
**次 session 担当**: r41 Phase 2 本実装着手前の最終 review + Phase 2 着手判断

---

## §1. 経緯

### §1.1 前 session 達成 (= 2026-06-06 前半)

前 session handoff `handoff/phase2-prep/handoff-phase2-prep-work-order-l4-complete.md` で:
- §3.5.7〜§3.5.16 起案 + §3.5.17 L4 サマリ起案完了 (= L4 全 16 group 62 UBO 起案完了 = C-6 完了)
- 39 file §12 反映 (= L4-7〜L4-16 sub-section 該当 UBO file)
- handoff doc 起案

### §1.2 本 session AYA literal 確定指示 (= 2026-06-06 後半、前 session 継承)

1. **C-8 + C-9 から着手 OK** (= 本 session 開始時確認、AYA「OK」)
2. **基本的に最初に伝えてる条件 (= 4 原則 + 視覚 regression ゼロ + AYA 既存機能維持) がクリアされるなら OK**
3. **現状の見た目とほぼ変わらない描画が望まれる** (= visual regression ゼロ §5.4 policy 再確認)
4. **問題発生時の解決は前提条件を破壊しないかぎり推奨案で OK** (= 設計判断 [要 AYA 判断] マークは推奨案を採用、AYA 検証時 reject なら再起案)
5. **進められるところまで進めてください** (= 自走承認継続)

### §1.3 本 session 着手前確認 = 必読 3 件 pinpoint Read 完了

- `design/ubo/WORK_ORDER.md` §4 既存 skeleton (line 3388-3399、C-8 で詳細展開対象) + §1 Layer 体系 + §2.5 L0 サマリ構造
- `design/09-phase-roadmap.md` §2.1 (line 67-87、訂正対象) + §2.2 直前 (line 89)
- 前 session handoff doc 全文 (= §3.1 C-8 起案内容 7 sub-section + §3.2 C-9 残 + §3.3 commit 提案)
- memory pinpoint = `project_r41_phase2_4_principles` (= §4 直接起案資料) + `project_ayastorm_r41_design_principles` (= 2 大設計原則) + `project_r41_phase1b_vulkan_host_gate` (= mUseUBO runtime gate)

---

## §2. 本 session 達成

### §2.1 起案完了 = C-8 + C-9 全完了

| # | task | 内容 | 規模 | 状態 |
|---|---|---|---|---|
| 1 | C-8 §4 起案 | `design/ubo/WORK_ORDER.md` §4 既存 skeleton (~12 行) → 詳細 7 sub-section (~160 行) | 大幅拡張 | **完了** |
| 2 | C-8 §2.1 訂正 | `design/09-phase-roadmap.md` §2.1.1 新規 sub-section (~50 行追加) | 追加 | **完了** |
| 3 | C-9 L3 §12 反映 | L3 20 file 末尾に §12 (= WORK_ORDER.md §3.4.N 同期) 一括 append | 20 file 各 ~20 行 | **完了** |

### §2.2 §4 起案内容詳細 (= ~160 行)

**起案位置**: `design/ubo/WORK_ORDER.md` §4 (= line 3388-、既存 skeleton 12 行を詳細 7 sub-section に置換)

**7 sub-section 構成**:

| § | 内容 | gate 件数 | 規模 |
|---|---|---|---|
| §4.1 | 原則 1: Core プロセス分散 (= C1-C6 設計制約) | 6 件 | ~25 行 |
| §4.2 | 原則 2: 3 OS 共通 (= OS-1〜OS-10 gate) | 10 件 | ~30 行 |
| §4.3 | 原則 3: Phase 2/3 範囲明確 (= R-1〜R-4 + Template A R3-R6 + O3-2 r42 移管) | 4 件 + Phase 表 | ~25 行 |
| §4.4 | 原則 4: OpenGL を殺さない (= O-1〜O-5 + `mUseUBO` runtime flag + `LL_VULKAN_GLSL` C++ 不使用) | 5 件 | ~25 行 |
| §4.5 | 視覚 regression ゼロ (= V-1〜V-4 + §5.4 policy cross-reference) | 4 件 + 含意 | ~25 行 |
| §4.6 | 各項目評価 protocol (= sub-work (7) 全 94 UBO + L0 4 protocol = 98 件) | 評価 cell 2842 | ~20 行 |
| §4.7 | violation 検知時の対応 (= stage 1 提案撤回 / stage 2 設計再考 / stage 3 AYA literal 確認) | 3 stage | ~25 行 |

**合計 29 gate × 94 UBO = 2726 件 + L0 4 件 = 2842 件 評価 cell** (= 全 UBO sub-work (7) で逐次 check)。

**起源 reference**:
- 4 原則: AYA literal 確定 2026-06-06 (= memory `project_r41_phase2_4_principles`)
- 視覚 regression ゼロ: AYA literal 2026-06-06「現状の見た目とほぼ変わらない描画」(= §5.4 policy)
- `mUseUBO` runtime gate: memory `project_r41_phase1b_vulkan_host_gate` (= `LL_VULKAN_GLSL` C++ 不使用、GLSL shader 側は引き続き有効)
- Phase 2/3 範囲 + Template A R3-R6: memory `project_ayastorm_r41_design_principles` + roadmap §5

### §2.3 09-phase-roadmap.md 訂正内容 (= §2.1 Phase 2..K split + §2.1.1 新規 + Phase K 整合化)

**起案位置**: `design/09-phase-roadmap.md` §2.1 Phase マップ表 + §2.1.1 新規 sub-section (= line 87 後、§2.2 直前)

**訂正 1**: §2.1 Phase マップ表「Phase 2..K」1 行 → **Phase 2 / Phase 3 / Phase 4 / Phase 5 / Phase 6..K** 5 行 split (= AYA literal 2026-06-06「UBO 調査前の Phase 2 作業範囲を UBO 1 個と書かれてしまったまま」指摘対応):
- Phase 2 = R3 単独 (= UB_REFLECTION_PROBES、1 UBO)
- Phase 3 = R4 + R5 bundle (= UB_GLTF_MATERIALS + PerDrawUBO_LightParams、**2 UBO 例外**)
- Phase 4 = Node 単独 (= Asset_GLTFNodes、1 UBO)
- Phase 5 = R6 単独 (= Skin_GLTFJoints + avatar Vulkan 通電、1 UBO)
- Phase 6..K = 残 88 UBO 順次 (= cluster 採否で K=93 程度 or 数十)

**訂正 2**: §2.1 Phase K 確定条件 (= line 75-80) を (Q1)(Q2) 一部確定状態 (= 2026-06-06) に整合化:
- (1) Phase 0 計測結果 = ✅ 完了 (= UBO 総数 = 94 UBO + L0 4 protocol 確定)
- (2) (Q1) 第 1 UBO 選定 = ✅ 確定 (= UB_REFLECTION_PROBES = Phase 2 R3)
- (3) (Q2) Phase 当たり UBO 数 = **一部確定** (= Phase 2-5 確定、Phase 6..K cluster 許可 = AYA 残判断)

**訂正 3**: §2.1 K = (Q1)(Q2) 確定後の表記 (= line 87) を「Phase 6..K cluster 許可 採否 = (Q2) 残判断」に整合化 + WORK_ORDER §4.3 双方向 link 明示

**§2.1.1 新規 sub-section 内容**:
- 双方向 link = 本 §2.1 (Phase 番号 source of truth) ↔ WORK_ORDER §3 (Phase 2..K sub-step 94 UBO 詳細起案)
- Layer 体系表 (= L0 4 protocol + L1a 3 + L1b 2 + L2 4 + L3 20 + L4 62 + L5 3 = 94 + L0 4 protocol)
- 役割分担明示 (= 本 chapter §2.1 + WORK_ORDER §3 + AYA literal 2026-06-06「Phase 2 着手前に Phase 2/3 詳細 scope 定義 separate session」整合)
- 4 原則 + 視覚 regression ゼロ gate cross-reference (= 原則 1 C1-C6 / 原則 2 OS-1〜OS-10 / 原則 3 R-1〜R-4 / 原則 4 O-1〜O-5 / V-1〜V-4)
- violation 検知 protocol cross-reference (= stage 1/2/3)

### §2.4 §12 反映 20 file 詳細 (= L3 全件)

各 file 末尾に WORK_ORDER §3.4.N (= L3-N) 同期 §12 を ~20 行 append:

| L3-N | UBO file |
|---|---|
| L3-1 | `PerProgramUBO_AlphaParams.md` (= near_clip 1 active) |
| L3-2 | `PerProgramUBO_PostDeferredV.md` (= tc_scale 1 active vec2) |
| L3-3 | `ScreenSpaceReflPostFParamUBO_Legacy.md` (= SSR zNear/zFar) |
| L3-4 | `SimpleColorFParamUBO_Legacy.md` (= waterSign 1 member) |
| L3-5 | `SnapshotFrameFParamUBO_Legacy.md` (= snapshot UI border) |
| L3-6 | `DofCombineFParamUBO_Legacy.md` (= DoF combine 3 float) |
| L3-7 | `NormgenFParamUBO_Legacy.md` (= bump-to-normal generation) |
| L3-8 | `AvatarClothVParamUBO_Legacy.md` (= cloth simulation 3 vec4) |
| L3-9 | `TerrainVParamUBO_Legacy.md` (= terrain texgen 2 vec4) |
| L3-10 | `PerProgramUBO_FullbrightShinyV.md` (= texture_matrix1 cubemap) |
| L3-11 | `PerProgramUBO_FxaaF.md` (= NVIDIA FXAA constant) |
| L3-12 | `PerProgramUBO_CofF.md` (= DoF Circle of Confusion 6 active) |
| L3-13 | `PerProgramUBO_BlurLightF.md` (= SSAO blur kernel) |
| L3-14 | `PerProgramUBO_VolumetricLightF.md` (= godray pipeline) |
| L3-15 | `PerProgramUBO_GodraysF.md` (= AYAstorm r15 godray cvar 3) |
| L3-16 | `PreviewVParamUBO_Legacy.md` (= preview render light array 768 B) |
| L3-17 | `SoftenLightParamUBO_Legacy.md` (= AYAstorm translucency + SSAO + blur) |
| L3-18 | `RlvFParamUBO_Legacy.md` (= RLVa Sphere effect) |
| L3-19 | `ShadowUtilParamUBO_Legacy.md` (= shadow_matrix[6] + bias、最大 512 B) |
| L3-20 | `GlobalFParamUBO_Legacy.md` (= mirror_flag/clipSign、shell + write 通電済) |

= 全 20 file 末尾 append + L3 全件共通 (4)(7) 簡潔記載 + 個別 (1)〜(6) 記載。

### §2.5 累積成果 = r41 Phase 2 前提条件 work 全完走

| C-N | task | 状態 | 完了 commit |
|---|---|---|---|
| C-1 | WORK_ORDER.md skeleton + §0 用語 + §1 Layer 体系 + §4/§5/§A skeleton | ✅ | `aa7803357b` |
| C-2 | §2 L0 横断 protocol 4 件 詳細 | ✅ | `f6e56bb7ea` (前々 commit) |
| C-3 | §3.1 L1a 3 UBO sub-work + §12 反映 | ✅ | `f6e56bb7ea` |
| C-4 | §3.2 L1b 2 UBO + §3.3 L2 4 UBO + §12 反映 | ✅ | `f6e56bb7ea` |
| C-5 | §3.4 L3 20 UBO sub-work (= L3 設計 task 起案) | ✅ | `f6e56bb7ea` (= sub-work 起案、§12 反映は本 session) |
| C-6 | §3.5 L4 16 group 62 UBO + §3.5.17 サマリ + §12 反映 | ✅ | `0808504ac2` (= §3.5.1-6 + 23 file §12) + 前 session (= §3.5.7-16 + §3.5.17 + 39 file §12、未 commit) |
| C-7 | §3.6 L5 3 UBO sub-work + §12 反映 | ✅ | `f6e56bb7ea` |
| C-8 | §4 4 原則 gate + 09-phase-roadmap §2.1.1 サマリ | ✅ | **本 session** (= 未 commit) |
| C-9 | 各 UBO file §12 反映 (= 94 file 全件) | ✅ | C-3〜C-7 commit + 前 session (L4 39 file) + **本 session L3 20 file** (= 累積 ~94 file 全件達成) |

**= r41 Phase 2 前提条件 work 全完走** (= 94 UBO + L0 4 protocol sub-work 起案 + 4 原則 + 視覚 regression ゼロ gate 起案 + roadmap 双方向 link + 各 UBO file §12 反映)。

---

## §3. 残作業 = なし、Phase 2 本実装着手判断

### §3.1 Phase 2 前提条件 work = 完了状態

C-1〜C-9 全完了。次の milestone は **r41 Phase 2 本実装着手** (= AYA literal 2026-06-06「Phase 2 着手前に Phase 2/3 詳細 scope 定義 separate session」整合)。

### §3.2 Phase 2 本実装着手前の最終 review check 項目 (= AYA literal 確認推奨)

| 項目 | 内容 | 確認位置 |
|---|---|---|
| 1 | L0 4 protocol AYA review 6 件 (= 設計判断) | WORK_ORDER §2.5.2 |
| 2 | 4 原則 + 視覚 regression ゼロ gate 全体構成 (= 5 軸 29 gate 評価 protocol) | WORK_ORDER §4 |
| 3 | 各 UBO §12 (= 94 file 全件、navigation 用) | 各 UBO file 末尾 §12 |
| 4 | Phase 2 着手 UBO 順序 (= Template A R3 = UB_REFLECTION_PROBES 単体から) | WORK_ORDER §3.5.9 + 09-phase-roadmap §2.1.1 |
| 5 | Phase 2 sub-step 命名規約 (= Phase 2.A / 2.B / 2.C) | WORK_ORDER §4.3 R-3 + 09-phase-roadmap §1.2 |
| 6 | 全 [要 AYA 判断] マーク全件 (= 各 UBO sub-work (2) 内) | grep `\[要 AYA 判断\]` でリストアップ可能 |

### §3.3 commit timing (= AYA 確認後)

- **直近**: 前 session 残 (= §3.5.7-16 + §3.5.17 + 39 file §12) + 本 session 全 (= §4 + §2.1.1 + L3 20 file §12) + 本 handoff doc = まとめて commit 推奨 (= **AYA commit 指示待ち**)

### §3.4 commit 提案 (AYA 判断)

| 案 | 内容 | 規模 | trade-off |
|---|---|---|---|
| 案 A | 前 session 残 + 本 session 全 = 1 commit (= L4 全 16 group 起案完了 + L3 20 file §12 + C-8 §4 + §2.1.1) | 大型 62 file 程度 | scope 単一 (= Phase 2 前提条件 work 完走) |
| 案 B | 前 session 残 を C-6-b〜C-6-k 10 commit + 本 session = 1 commit | 11 commit | 細粒度だが事後 review コスト高 |
| 案 C | 前 session 残 = 1 commit (C-6-b〜k bundle) + 本 session = 1 commit (C-8/C-9) | 2 commit | scope 中粒度 (= L4 完了 + Phase 2 前提条件全完走 を分離) |

**推奨**: **案 C** (= 2 commit、L4 完了 と Phase 2 前提条件全完走 を分離して commit message 明確化、handoff 切替境界に対応)

---

## §4. 必読 (= 次 session 着手前、memory `feedback_handoff_minimal_pre_req_read` 適用)

**最低限 3 件**:
1. `design/ubo/WORK_ORDER.md` (= 全 §3 + §4 + §5 = Phase 2 着手前 master doc、必読)
2. `design/09-phase-roadmap.md` §2.1 + §2.1.1 (= Phase 番号体系 + WORK_ORDER 双方向 link)
3. 本 handoff doc (= Phase 2 前提条件 work 完走 record)

**pinpoint Read (必要時)**:
- memory `project_r41_phase2_4_principles` (= 4 原則 確定 record)
- memory `project_ayastorm_r41_design_principles` (= r41 全体 2 大設計原則)
- memory `project_r41_phase1b_vulkan_host_gate` (= `mUseUBO` runtime flag、`LL_VULKAN_GLSL` C++ 不使用)
- 各 UBO file §12 (= 該当 UBO Phase 2 本実装着手時)
- 前 session handoff `handoff-phase2-prep-work-order-l4-complete.md` §3.1 = C-8 起案内容 7 sub-section の根拠
- 前々 session handoff `handoff-phase2-prep-work-order-l0-l3-l5-progress.md` (= 必要時 historical reference)

**memory 必読**:
- `project_r41_phase2_4_principles` (= 4 原則 確定 record)
- `feedback_admit_unknown` (= 推論禁止、不明明示)
- `feedback_design_phase_no_code_write` (= `indra/` 改変ゼロ、design-phase 規律 = 本 session も継続遵守)
- `feedback_no_scope_shrink` (= AYA literal scope 厳守)
- `feedback_proactive_handoff` (= context 圧迫時自分から引き継ぎ提案)
- `feedback_handoff_minimal_pre_req_read` (= 最低限 3 件 + pinpoint Read)
- `feedback_ubo_migration_one_at_a_time` (= Phase 2 本実装着手後の規律 = 1 UBO ずつ + cold launch 検証)
- `feedback_build_only_verified` (= Phase 2 本実装着手後の規律 = 実機検証 only、効果未確認は積まない)
- `feedback_no_claude_coauthor` (= 全 commit message で Claude 共著行を含まない)

---

## §5. 起案規律 (= 本 session 経験継承 + Phase 2 本実装着手後継続)

### §5.1 必須遵守 (= 本 session 達成 + 継承)

- **1 group ずつ起案 + §12 反映** (= C-6 L4 16 group、C-9 L3 20 file 全完了、本 session 経験から効率的)
- **推論禁止、不明明示** (= memory `feedback_admit_unknown`、[要追加調査] / [要 verify] / [要 AYA 判断] / [要 L0-4 結果反映] マーク継承)
- **4 原則 死守** + **視覚 regression ゼロ** (= 全項目 sub-work (6)(7) で評価、AYA literal「現状の見た目とほぼ変わらない描画」継承)
- **命名統一** (= L4-N group/UBO 番号 + AYA literal group 名 + READINESS §3.N mapping 併記)
- **`indra/` 改変ゼロ** (= design-phase 規律、本 session 完全遵守)
- **AYA literal scope 厳守** (= AYA literal「推奨案で OK」採用、4 原則 violation 提案禁止)
- **各 UBO 作業完了で §12 反映** (= 軽量版 template、single source of truth = WORK_ORDER.md §3.N.M)

### §5.2 context 容量管理 (= 本 session の教訓)

- **本 session 達成**: §4 起案 (= ~160 行) + §2.1.1 起案 (= ~50 行) + L3 20 file §12 (= 一括 Bash append、~400 行) = 計 ~610 行追加
- **必読 Read 効率化**: handoff doc + 既存 §4 skeleton + Layer 体系 + §2.5 サマリ + memory 3 件 = 5-7 file pinpoint Read で完走
- **context 効率化技法**:
  - 一括 Bash append (= 20 file 1 command) で個別 Edit より高効率
  - WORK_ORDER §1 + §2.5 (= 既起案 sample) を参照して §4 構造を統一
- **70-80% 消費で handoff 切替推奨** (= 本 session ~85% で handoff 起案、規律内)

### §5.3 Phase 2 本実装着手時の規律 (= 次々 session 以降)

- **AYA literal「Phase 2 着手前に Phase 2/3 詳細 scope 定義 separate session」遵守** (= 2026-06-06)
- **Phase 2 = UB_REFLECTION_PROBES 単体実装** (= Template A R3 のみ、(Q2) A 1 UBO 厳守維持、cluster 例外なし)
- **Phase 2.A / 2.B / 2.C 命名規約** (= PC-N-* 体系は Phase 1.E で役目終了、Phase 2 以降は handoff sub-letter)
- **`mUseUBO` runtime flag default OFF** (= dual-path 出荷、OpenGL path 維持 + Vulkan path opt-in)
- **`LL_VULKAN_GLSL` C++ 不使用、GLSL shader 側は引き続き有効** (= memory `project_r41_phase1b_vulkan_host_gate`)
- **AYA 検証時 reject 時の対応**: sub-work (7) violation 判定 → §4.7 protocol 適用 (= stage 1 提案撤回 / stage 2 設計再考 / stage 3 AYA literal 確認)
- **memory `feedback_ubo_migration_one_at_a_time`**: 1 UBO ずつ実装 → cold launch 検証 PASS → 次へ
- **memory `feedback_build_only_verified`**: 効果未確認 commit / 設計倒れ spec を積まない、確認できないものは revert する勇気

---

## §A. 本 commit 対象 (= AYA literal 承認後想定、案 C 推奨想定)

### §A.1 commit-1 候補 (= 前 session 残 = L4 完了 bundle)

| file | 種別 | 内容 |
|---|---|---|
| `design/ubo/WORK_ORDER.md` | 訂正 | §3.5.7〜§3.5.16 起案 10 group 37 UBO + §3.5.17 L4 サマリ (= ~2200 行追加) |
| `design/ubo/{FrameAtmosphere_Lighting,SkyVParamUBO_Legacy,SkyFParamUBO_Legacy,CloudsVParamUBO_Legacy,CloudsFParamUBO_Legacy,StarsFParamUBO_Legacy,StarsVParamUBO_Legacy,SunDiscFParamUBO_Legacy,MoonFParamUBO_Legacy,AtmoExtraUBO_Legacy}.md` | 訂正 | §12 反映 (= L4-7、AtmoExtra cross-group ゆえ 10 entry 9 file 新規 + 1 cross-update) |
| `design/ubo/{PerDrawUBO_AvatarSkin,PerDrawUBO_AvatarVelocity,PerDrawUBO_ObjectSkin,PerDrawUBO_SkinnedVelocity,VelocityVParamUBO_Legacy,PerProgramUBO_VelocityAlphaV}.md` | 訂正 | §12 反映 (= L4-8) |
| `design/ubo/{Global_ReflectionProbes,ReflectionProbeUBO_Legacy,RadianceGenFParamUBO_Legacy,IrradianceGenFParamUBO_Legacy,GaussianFParamUBO_Legacy}.md` | 訂正 | §12 反映 (= L4-9) |
| `design/ubo/{ExposureFParamUBO_Legacy,LuminanceFParamUBO_Legacy,TonemapUBO_Legacy,PerProgramUBO_GammaCorrect,PerProgramUBO_ColorGrading,VignetteParamUBO_Legacy}.md` | 訂正 | §12 反映 (= L4-10) |
| `design/ubo/{GlowExtractFParamUBO_Legacy,GlowVParamUBO_Legacy,GlowFParamUBO_Legacy,GlowCombineFParamUBO_Legacy}.md` | 訂正 | §12 反映 (= L4-11) |
| `design/ubo/{SMAAParamUBO_Legacy,SMAABlendWeightsFParamUBO_Legacy}.md` | 訂正 | §12 反映 (= L4-12) |
| `design/ubo/{PathfindingVParamUBO_Legacy,PathfindingNoNormalVParamUBO_Legacy}.md` | 訂正 | §12 反映 (= L4-13) |
| `design/ubo/{Asset_GLTFMaterials,Asset_GLTFNodes}.md` | 訂正 | §12 反映 (= L4-14) |
| `design/ubo/{PerDrawUBO_ClipPlane,PerDrawUBO_LightParams}.md` | 訂正 | §12 反映 (= L4-15) |
| `design/ubo/PerDrawUBO_MultiLight.md` | 訂正 | §12 反映 (= L4-16) |
| `handoff/phase2-prep/handoff-phase2-prep-work-order-l4-complete.md` | 新規 | 前 session handoff |

合計 ~41 file (= 1 WORK_ORDER 訂正 + 39 §12 反映 + 1 前 session handoff)。

### §A.2 commit-2 候補 (= 本 session = C-8 + C-9 + 本 handoff)

| file | 種別 | 内容 |
|---|---|---|
| `design/ubo/WORK_ORDER.md` | 訂正 | §4 詳細起案 (= ~160 行、既存 12 行 skeleton → 7 sub-section) |
| `design/09-phase-roadmap.md` | 訂正 | §2.1.1 新規 sub-section (= ~50 行) |
| `design/ubo/{PerProgramUBO_AlphaParams,PerProgramUBO_PostDeferredV,ScreenSpaceReflPostFParamUBO_Legacy,SimpleColorFParamUBO_Legacy,SnapshotFrameFParamUBO_Legacy,DofCombineFParamUBO_Legacy,NormgenFParamUBO_Legacy,AvatarClothVParamUBO_Legacy,TerrainVParamUBO_Legacy,PerProgramUBO_FullbrightShinyV,PerProgramUBO_FxaaF,PerProgramUBO_CofF,PerProgramUBO_BlurLightF,PerProgramUBO_VolumetricLightF,PerProgramUBO_GodraysF,PreviewVParamUBO_Legacy,SoftenLightParamUBO_Legacy,RlvFParamUBO_Legacy,ShadowUtilParamUBO_Legacy,GlobalFParamUBO_Legacy}.md` | 訂正 | §12 反映 (= L3-1〜L3-20) |
| `handoff/phase2-prep/handoff-phase2-prep-complete.md` | 新規 | 本 handoff |

合計 ~23 file (= 1 WORK_ORDER + 1 09-phase-roadmap + 20 §12 + 1 handoff)。

### §A.3 案 A 統合 1 commit 案 (= 案 C 不採用時)

= §A.1 + §A.2 = 計 ~64 file。1 commit、scope = Phase 2 前提条件 work 完走。

### §A.4 commit message 提案 (= 案 C 想定)

**commit-1** (= 前 session 残 = L4 完了 bundle):

```
docs: r41 Phase 2 前提条件 WORK_ORDER.md C-6 L4 全 16 group 起案完了 (= 62/62 UBO + 39 file §12 + L4 サマリ)
```

**commit-2** (= 本 session = Phase 2 前提条件 全完走):

```
docs: r41 Phase 2 前提条件 work 全完走 (= C-8 §4 4 原則 gate + 09-phase-roadmap §2.1.1 + C-9 L3 20 file §12 + 94 UBO 累積完走)
```

Co-Authored-By 行不在 (= memory `feedback_no_claude_coauthor` 遵守)。

---

## §B. 次 session 着手手順

1. 上記必読 3 件 pinpoint Read (= WORK_ORDER.md + 09-phase-roadmap §2.1.1 + 本 handoff)
2. memory pinpoint 確認 (= project_r41_phase2_4_principles + project_ayastorm_r41_design_principles + project_r41_phase1b_vulkan_host_gate + feedback_ubo_migration_one_at_a_time + feedback_build_only_verified)
3. AYA literal 確認 (= §3.2 最終 review check 6 項目 + commit 案 A/B/C 選択)
4. AYA 承認後 commit 実施 (= 案 C 推奨 = 2 commit)
5. r41 Phase 2 本実装着手判断 → Phase 2 = UB_REFLECTION_PROBES 単体実装 separate session entry

**次 session 開始時の最初の AYA 確認**: 「上記要約を確認、Phase 2 前提条件 work 全完走を commit して、Phase 2 本実装 = UB_REFLECTION_PROBES 単体実装 separate session entry に進むで OK か?」
