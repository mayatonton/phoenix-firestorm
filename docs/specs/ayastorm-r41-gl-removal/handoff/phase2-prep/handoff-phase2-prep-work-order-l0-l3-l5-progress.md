# r41 Phase 2 前提条件 WORK_ORDER.md 起案 — L0/L1a/L1b/L2/L3/L5 完了 + L4/C-8/C-9 残 次 session 引継ぎ

**起案契機**: 2026-06-06 AYA literal「進められるところまで進めてください (C-9 まで OK)」record 自走承認、context 圧迫前に handoff 起案。

**位置付け**: WORK_ORDER.md 起案 work 中盤の進捗 record + 次 session 持越作業詳細化。

---

## §1. 経緯

### §1.1 前 session handoff record

- 前 session 起案: `handoff/phase2-prep/handoff-phase2-prep-relations-readiness-complete.md` (= RELATIONS.md + READINESS.md 起案完了)
- 前 session commit: aa7803357b (= UBO design 95 file 起案完了)

### §1.2 本 session AYA literal 確定指示

1. **Phase 2 作業項目命名** = A-1, B-1〜B-31, C-1〜C-62
2. **trace 順序** = 単純配列でなく **条件が揃っていく過程で進む順**
3. **各項目に sub-work 詳細** (= 7 dimension template)
4. **設計 + 工程 2 軸で資料化**
5. **case C 案採用** = `09-phase-roadmap.md` §2.1 サマリ + `design/ubo/WORK_ORDER.md` 詳細切出
6. **各 UBO 作業完了で UBO file §12 反映**
7. **visual regression ゼロ** (= 「現状の見た目とほぼ変わらない描画が望まれる」、AYA literal 2026-06-06 追加条件)
8. **4 原則 死守** = 3 OS 共通 / Core 分散 / Phase 範囲 / OpenGL を殺さない
9. **進められるところまで進めてください (C-9 まで OK)** = 自走承認

---

## §2. 本 session 達成

### §2.1 起案完了 file

| file | 状態 | 内容 |
|---|---|---|
| `design/ubo/WORK_ORDER.md` (新規) | **C-1〜C-5 + C-7 完了 (= 32 UBO + 4 protocol sub-work 起案完了)** | §0 用語 + §1 Layer 体系 + §2 L0 4 protocol + §3.1 L1a 3 + §3.2 L1b 2 + §3.3 L2 4 + §3.4 L3 20 + §3.6 L5 3 + §4/§5/§A skeleton + §5.4 visual regression policy |
| `<UBO名>.md` § 12 反映 (= 9 file 完了) | **C-9 部分完了** | L1a 3 (CAS/Clip/VisualizeBuffersF) + L1b 2 (FrameViewProj/FrameLights) + L2 4 (PbrTerrainV/AOUtil/MotionBlur/DeferredUtil) + L5 3 (Skin_GLTFJoints/FsObjectIdF/NormaldebugV) = 12 file 反映 |

### §2.2 完了 commit 単位

| Commit | 内容 | status |
|---|---|---|
| **C-1** | WORK_ORDER.md skeleton + §0 用語 + §1 Layer 体系 + §4/§5/§A skeleton | ✅ 完了 |
| **C-2** | §2 L0 横断 protocol 4 件 詳細 (= name-based dispatch / LLStaticHashedString redirect / per-shader UBO block 拡大 / cadence 再評価) | ✅ 完了 |
| **C-3** | §3.1 L1a 3 UBO (= CAS / Clip / VisualizeBuffersF) + L1a 3 file §12 反映 | ✅ 完了 |
| **C-4** | §3.2 L1b 2 UBO (= FrameViewProj / FrameLights) + §3.3 L2 4 UBO (= PbrTerrainV / AOUtil / MotionBlur / DeferredUtil) + 6 file §12 反映 | ✅ 完了 |
| **C-5** | §3.4 L3 20 UBO sub-work (= AlphaParams / PostDeferredV / ScreenSpaceReflPostF / SimpleColor / SnapshotFrame / DofCombine / Normgen / AvatarClothV / TerrainV / FullbrightShinyV / FxaaF / CofF / BlurLightF / VolumetricLightF / GodraysF / PreviewV / SoftenLight / RlvF / ShadowUtil / GlobalF) | ✅ 完了 (= L3 file §12 反映は次 session 持越) |
| **C-7** | §3.6 L5 3 UBO (= Skin_GLTFJoints A-1 + FsObjectIdF + NormaldebugV) + L5 3 file §12 反映 | ✅ 完了 |

### §2.3 §1.1 Layer 体系訂正 (= AYA literal 94 項目維持)

| Layer | 件数 (確定) | UBO 列挙 |
|---|---|---|
| **L0** | 4 protocol | name-based dispatch / LLStaticHashedString redirect / per-shader UBO block 拡大 / cadence 再評価 |
| **L1a** | 3 | CAS / Clip / VisualizeBuffersF |
| **L1b** | 2 | FrameViewProj / FrameLights |
| **L2** | 4 | PbrTerrainV / AOUtil / MotionBlur / DeferredUtil |
| **L3** | 20 | (= READINESS §4 B 残 20 件、§3.4.21.1 サマリ参照) |
| **L4** | **62 (= 16 group、未起案)** | (= C-6 持越、§3.5 placeholder) |
| **L5** | 3 | Skin_GLTFJoints A-1 + FsObjectIdF + NormaldebugV |

合計 = 3 + 2 + 4 + 20 + 62 + 3 = **94 件** ✅ (= AYA literal 94 項目維持、INDEX §2 一致)

---

## §3. 残作業 = 次 session 実施

### §3.1 C-6: §3.5 L4 C 16 group trace 順 + sub-work 起案 (= 大型)

**file**: `design/ubo/WORK_ORDER.md` §3.5 placeholder 置換
**規模**: 62 UBO × 16 group、~2500-3000 行追加見込
**16 group**:
- §3.5.1 aya_sss_skin_flag 3 UBO (= MaterialUBO_Legacy + PBROpaqueExtraUBO_Legacy + AvatarFParamUBO_Legacy)
- §3.5.2 aya_visual_realism + chroma_str + light cvar 7 UBO (= AtmoExtraUBO_Legacy + SkinSSSPrototypeFParamUBO_Legacy + PostDeferredF/NoDoFF + PointLightF/SpotLightF/PointLightV)
- §3.5.3 shadow_target_width 3 UBO (= ShadowAlphaMaskV + PbrShadowAlphaMaskV + AvatarAlphaShadowV)
- §3.5.4 box_center/box_size 2 UBO (= OcclusionCubeV + ShadowCubeV)
- §3.5.5 GLTF texture transform 3 UBO (= PbrOpaqueV + PbrAlphaV + MaterialUBO)
- §3.5.6 water 系 5 UBO (= WaterFog + WaterV + UnderWater + WaterF + WaterHazeV)
- §3.5.7 sky/cloud/atmospheric 10 UBO (= FrameAtmosphere_Lighting + AtmoExtra + Sky V/F + Clouds V/F + Stars F/V + SunDisc + Moon)
- §3.5.8 velocity 5 UBO (= AvatarSkin + AvatarVelocity + ObjectSkin + SkinnedVelocity + VelocityVParam + VelocityAlphaV)
- §3.5.9 reflection probe / IBL 5 UBO (= Global_ReflectionProbes + ReflectionProbe + RadianceGen + IrradianceGen + Gaussian)
- §3.5.10 post-process chain 6 UBO (= Exposure + Luminance + Tonemap + GammaCorrect + ColorGrading + Vignette)
- §3.5.11 glow chain 4 UBO (= GlowExtract + GlowV + GlowF + GlowCombine)
- §3.5.12 SMAA 2 UBO (= SMAAParam + SMAABlendWeights)
- §3.5.13 pathfinding 2 UBO (= Pathfinding V + NoNormal)
- §3.5.14 GLTF asset 2 UBO (= Asset_GLTFMaterials + Asset_GLTFNodes)
- §3.5.15 set=2 binding=0 共有 (残) 2 UBO (= ClipPlane + LightParams)
- §3.5.16 MultiLight 1 UBO (= MultiLight)

**起案手順**:
1. 各 group 内 UBO file pinpoint Read (= 1 group 5-10 file)
2. group 単位で sub-work 7 dim 起案 (= cross-UBO 同期 protocol を group 単位で確立)
3. **verify 単位 = group verify** (= group 全件揃って初めて整合 visual)
4. 各 group 内 UBO 間並列性 + group 間並列性記載
5. group 内 UBO file §12 反映 (= 並走 C-9)

**memory `feedback_proactive_handoff` 適用**: C-6 は 1 session で完了困難 (= 推定 ~2-3 session)、各 group 完了時点で commit + handoff 更新推奨。

### §3.2 C-8: §4 4 原則 gate + 09-phase-roadmap.md §2.1 サマリ訂正

**file**: `design/ubo/WORK_ORDER.md` §4 + `design/09-phase-roadmap.md` §2.1 訂正

**§4 起案内容**:
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

**完了済 (= 12 file)**: L1a 3 + L1b 2 + L2 4 + L5 3 = 12 file
**残 (= ~82 file)**:
- **L3 20 file** (= 本 session 起案済、§12 反映のみ残)
- **L4 62 file** (= C-6 起案中 / 完了後に並走)

**反映内容 template** (= 軽量版、~15 行):
```markdown

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.X.Y 同期)

**Layer**: L<N>-<M> (= <Layer 定義>)
**status**: **起案済** (= 2026-06-06 C-<commit>、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.X.Y` (= single source of truth)
**要点**: trace L<N>-<M>、工数 <S/M/L>、setter <状況>、AYA live verify (= visual regression ゼロ §5.4)
**関連**: L0-N (= WORK_ORDER §2) / §5.4 visual regression policy / <他関連>
```

### §3.4 commit timing (= AYA 確認後)

- 推奨 = 本 handoff + C-1〜C-5 + C-7 内容を 1 commit にまとめて記録
- C-6 起案途中で適宜 commit (= group 単位)
- 全完了後に最終 commit (= C-8 + C-9 残)

---

## §4. 必読 4 件 (= 次 session 着手前)

1. `design/ubo/WORK_ORDER.md` (= **必読**、本 phase 起案 main file、§3.5 placeholder 確認 + §3 全体構成把握)
2. `design/ubo/READINESS.md` §3 (= C 62 件 16 group 詳細、L4 起案の source)
3. `design/ubo/RELATIONS.md` §5 dirty 連動 group 20+ (= L4 内 cross-UBO 同期設計の source)
4. 本 handoff doc (= 進捗 + 残作業)

**memory 必読**:
- `project_r41_phase2_4_principles` (= 4 原則 確定 record)
- `feedback_admit_unknown` (= 推論禁止、不明明示)
- `feedback_design_phase_no_code_write` (= `indra/` 改変ゼロ)
- `feedback_no_scope_shrink` (= AYA literal scope 厳守)
- `feedback_proactive_handoff` (= context 圧迫時自分から引き継ぎ提案)

---

## §5. 起案規律 (= 絶対遵守、本 session 経験継承)

### §5.1 必須遵守

- **1 UBO 1 read 順次精査** (= 設計検討時 pinpoint Read、20 件超は batch 5-10 file ずつ並列)
- **推論禁止、不明明示** (= memory `feedback_admit_unknown`、[要追加調査] / [要 verify] / [要 AYA 判断] マーク付け)
- **4 原則 死守** + **visual regression ゼロ** (= 全項目 sub-work (6)(7) で評価)
- **命名統一** (= A-1 / B-1〜B-31 / C-1〜C-62 literal 維持、AYA 単純配列番号 + L0-L5 trace 順番号併記)
- **`indra/` 改変ゼロ** (= design-phase 規律)
- **AYA literal scope 厳守** (= 4 原則 violation 提案禁止、scope 縮小・拡大禁止)
- **各 UBO 作業完了で §12 反映** (= single source of truth = WORK_ORDER.md §3、§12 = 個別 UBO ナビゲーション向け軽量版)

### §5.2 context 容量管理 (= 本 session の教訓)

- C-6 L4 起案は 1 session で完了困難 (= 推定 2-3 session)
- 各 group 完了時点で commit + handoff 更新推奨
- context 残量を能動監視 (= 70-80% 消費で handoff 切替)
- file §12 反映は軽量版 template (= 推奨 ~15 行 / file) で context 節約

### §5.3 L4 起案の特異点

- **cross-UBO 同期 protocol を group 単位で確立** (= 各 group 内 UBO 間 dirty 連動 + data source 共有)
- **verify 単位 = group verify** (= group 全件揃って初めて整合 visual、中間状態は暫定 default 値で破綻回避)
- **AYA 既存機能 risk 大** (= aya_sss_skin_flag / aya_visual_realism / r30 Cinematic 13 cvar 関連)、機能維持必須
- **L0-4 cadence 再評価結果に依存する group 多数** (= velocity / GLTF material / post-process chain 等)

---

## §A. 本 commit 対象 (= AYA literal 承認後)

| file | 種別 | 内容 |
|---|---|---|
| `design/ubo/WORK_ORDER.md` | 新規 | C-1 skeleton + C-2 L0 + C-3 L1a + C-4 L1b/L2 + C-5 L3 + C-7 L5 全件 |
| `design/ubo/ClipFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L1a-1) |
| `design/ubo/CASParamUBO_Legacy.md` | 訂正 | §12 追加 (= L1a-2) |
| `design/ubo/PerProgramUBO_VisualizeBuffersF.md` | 訂正 | §12 追加 (= L1a-3) |
| `design/ubo/FrameViewProj.md` | 訂正 | §12 追加 (= L1b-1) |
| `design/ubo/FrameLights.md` | 訂正 | §12 追加 (= L1b-2) |
| `design/ubo/PerProgramUBO_PbrTerrainV.md` | 訂正 | §12 追加 (= L2-1) |
| `design/ubo/AOUtilParamUBO_Legacy.md` | 訂正 | §12 追加 (= L2-2) |
| `design/ubo/MotionBlurFParamUBO_Legacy.md` | 訂正 | §12 追加 (= L2-3) |
| `design/ubo/DeferredUtilParamUBO_Legacy.md` | 訂正 | §12 追加 (= L2-4) |
| `design/ubo/Skin_GLTFJoints.md` | 訂正 | §12 追加 (= L5-1 / A-1) |
| `design/ubo/PerProgramUBO_FsObjectIdF.md` | 訂正 | §12 追加 (= L5-2) |
| `design/ubo/NormaldebugVParamUBO_Legacy.md` | 訂正 | §12 追加 (= L5-3) |
| `handoff/phase2-prep/handoff-phase2-prep-work-order-l0-l3-l5-progress.md` | 新規 | 本 handoff |

合計 14 file (= 1 新規 + 12 §12 反映 + 1 handoff)。
Co-Authored-By 行不在 (= memory `feedback_no_claude_coauthor` 遵守)。
