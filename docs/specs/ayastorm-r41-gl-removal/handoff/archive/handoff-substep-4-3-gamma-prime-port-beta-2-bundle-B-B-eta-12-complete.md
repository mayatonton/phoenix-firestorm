# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-12 完遂 handoff

**status**: B?-η-12 完遂 → 次 sub-bundle B?-η-13 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: (η-11 patch `9a271fc900` 範式継承、AYA 「commit して」明示指示下 commit 予定 2026-06-02)
**handoff doc commit**: 本 doc (η-11-complete `74705c35bb` 範式継承、別 commit 予定)
**勤続範式継承**: B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-12 scope = **NEW 主指標 `overlapping use of location 21` 4 件解消** (AYA「B' 単独で進めて」指示確定 2026-06-02、η-11 handoff §10.1 漏れ発見 + η-11 §3.4 cascade pair shift 順方向 ACCEPT 判定範式 第 2 例として scope 化)。

- **Phase 単一 (1-shot ACCEPT)**: NEW overlapping=21 重複の **3 V file + 4 F file = 7 file 同期** 整合 = `layout(location=21)` → `layout(location=27)` numeric-only
  - 4 program (Underwater + EnvMap + WLSky + WLCloud) は全て **η-11 で patch した 7 V file の subset**、η-11 patch (loc=20→26) で loc=21 を見落とした **不完全 patch の cascade 露出**
  - canonical = `atmosphericsVarsV.glsl:33` vary_AtmosAttenuation (~25 shader 共有、helper auto-attach 経由、η-11 §3.1 第 3 例の auto-attach helper 経路 canonical 同定範式と同系統)
  - patch 対象 = waterV + skyV + cloudsV (3 V file) + waterF + underWaterF + skyF + cloudsF (4 F file、underWaterF は waterV の第 2 F partner = η-9 §3.1 V/F pair canonical partner 同定範式 適用)
  - η-9 §3.1 V/F pair canonical partner 同定範式 **直接適用第 4 例**
  - η-11 §3.4 cascade pair shift 順方向 ACCEPT 判定範式 **直接適用第 2 例**

**計**: **7 file +7/-7** (shader file のみ、C++ touch 0)

**主指標達成** (vs η-11 baseline):
- `overlapping use of location 21` 4 → **0** ✓ **-4 / 100% 完全達成**
- `overlapping use of location` (全件) 4 → 0 ✓

**既達退行検出** (cascade pair shift 順方向、Y = X = 4 境界条件、AYA「ACCEPT して η-13 移管」judgment 確定 2026-06-02):
- `undeclared identifier` 0 → **1** (Deferred Windlight Cloud Program line 1283 'cloud_scale')
- `nameless block ... global scope` 0 → **2** (Environment Map Program + Deferred Windlight Sky Shader = `FrameAtmosphere_Skybox` member × global scope 衝突)
- `non-opaque uniforms outside a block` 5 → **6** (+1、Underwater Shader line 1099)
- 計 Y = 4 件、X = 4 件、**Y = X 境界条件**

**他既達主指標完全維持** (η-12 末): `Layout location qualifier must match` 0 / `'binding'` 0 / `Cannot reuse block name` 0 / `'weight4' redefinition` 0 / `'weight' redefinition` 0 / `Link failed` 0 / `'size' undeclared` 0 / `GBufferInfo redefinition struct` 0 / `'#'` 0 / `'normalMap' redefinition` 69 維持 / `'depthMap' redefinition` 9 維持 / `'location'` literal 0 維持

**Phase 構成の特徴**: η-11 と同様 **Phase 1 即 ACCEPT の 1-shot 完結**。η-9 §3.1 + η-11 §3.4 範式適用で canonical partner + 既達退行 ACCEPT 判定を着手前 trace で確定 → falsification 不要で直接 deliverable。本 sub-bundle 特徴 = **handoff doc 記録漏れ発見** (η-11 §10.1 = AYA メッセージで「Layout 残 3」と記載されたが、実 log では NEW overlapping=21 4 件) = **handoff doc canonical 記載 着手前再検証範式 (η-10 §3.1 / η-11 §3.2)** の **記録漏れ補完範式新規 (§3.4)**。

---

## §2 完遂結果 metric (vs B?-η-11 baseline commit `9a271fc900`)

| metric | η-11 baseline | η-12 (verify) | Δ vs η-11 | 判定 |
|---|---|---|---|---|
| **overlapping use of location 21** | **4** | **0** | **-4** | ✓ **100% 完全達成** |
| overlapping use of location (全件) | 4 | 0 | -4 | ✓ |
| **undeclared identifier** | **0** | **1** | **+1** | ⚠️ **既達退行** (WLCloud cloud_scale、cascade pair shift 順方向、Y=X=4 境界 ACCEPT) |
| **nameless block ... global scope** | **0** | **2** | **+2** | ⚠️ **既達退行** (EnvMap + WLSky FrameAtmosphere_Skybox、cascade pair shift 順方向、Y=X=4 境界 ACCEPT) |
| **non-opaque uniforms outside a block** | **5** | **6** | **+1** | ⚠️ **既達退行** (Underwater line 1099、cascade pair shift 順方向、Y=X=4 境界 ACCEPT) |
| Layout location qualifier must match | 0 | 0 | ±0 | ✓ η-10 達成維持 |
| 'binding' | 0 | 0 | ±0 | ✓ η-8 達成維持 |
| Cannot reuse block name | 0 | 0 | ±0 | ✓ η-1 達成維持 |
| 'weight4' redefinition | 0 | 0 | ±0 | ✓ η-5 達成維持 |
| 'weight' redefinition | 0 | 0 | ±0 | ✓ η-7 達成維持 |
| Link failed | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| 'size' undeclared | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| '#' preprocessor | 0 | 0 | ±0 | ✓ ε 達成維持 |
| SPIR-V requires location | 41 | 41 | ±0 | ✓ (η-13+ 移管継続) |
| 'normalMap' redefinition | 69 | 69 | ±0 | ✓ (η-13+ 移管継続) |
| 'depthMap' redefinition | 9 | 9 | ±0 | ✓ (η-13+ 移管継続) |
| 'location' literal grep | 0 | 0 | ±0 | ✓ (Layout 0 + overlapping 0 + SPIR-V 41 全て literal 'location' 経由なしの error category) |
| parse failed | 130 | 130 | ±0 | (cascade pair shift 内訳のみ、parse failed program 数は同等維持) |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | ±0 | ✓ |
| Goodbye | 1 | 1 | ±0 | ✓ clean shutdown |
| Vulkan device destroyed | 1 | 1 | ±0 | ✓ clean shutdown |

**主指標 overlapping use of location 21 100% 完全達成 + 12 種既達主指標完全維持 + 既達退行 3 系統計 4 件 AYA「ACCEPT して η-13 移管」judgment 確定** = η-12 主 scope 完全達成、退行 4 件 (undeclared 1 + nameless 2 + non-opaque 増分 1) は **B?-η-13 移管**。

---

## §3 設計範式

### §3.1 設計範式継承: η-9 §3.1 V/F pair canonical partner 同定範式 第 4 例

**概要**: η-9 で確立された V/F pair canonical partner 同定範式を **第 4 例として直接適用**。範式自体に変更なし、適用 file 群と program 群のみ拡大:

| 適用例 | 適用 sub-bundle | program 群 | canonical (変更不可) | patch 対象 (1 program 専用または狭範囲) | 結果 |
|---|---|---|---|---|---|
| 第 1 例 | η-9 Phase 2 | gDeferredAvatarEyesProgram | diffuseF.glsl | avatarEyesV.glsl (4/2/0 → 0/1/2) | Layout 12→3 (-9) |
| 第 2 例 | η-10 | gDeferredNonIndexedDiffuseAlphaMaskProgram | diffuseV.glsl | diffuseAlphaMaskF.glsl (4/2/0 → 0/1/2) | Layout 3→0 (-3) |
| 第 3 例 | η-11 | 11 isDeferred program 群 | atmosphericsVarsV.glsl:28 vary_AdditiveColor (~25 shader 共有、auto-attach helper 経由) | 7 V + 8 F = 15 file (loc=20→26) | overlapping=20 11→0 (-11) |
| **第 4 例** | **η-12** | **4 isDeferred program (Underwater + EnvMap + WLSky + WLCloud)** | **atmosphericsVarsV.glsl:33 vary_AtmosAttenuation (~25 shader 共有、auto-attach helper 経由)** | **3 V + 4 F = 7 file (loc=21→27、underWaterF は waterV 第 2 F partner)** | **overlapping=21 4→0 (-4)** |

**第 4 例の特徴**: η-11 第 3 例で確立した auto-attach helper 経路 canonical 同定範式の **連続適用** = canonical atmosphericsVars helper file は loc=20 + loc=21 の **2 var** を持つので、main shader 側の loc=20 + loc=21 同時整合が本来必要だった = η-11 で loc=20 のみ整合させた結果、η-12 で loc=21 cascade 露出 = **複数 var 持つ canonical helper の整合は同 sub-bundle 内で完結が望ましい**。

**charter §3 #1 担保**: 全 7 file の GL `#else` path `out/in <type> <var>;` 元宣言不変、Vulkan path 内 `layout(location=N)` の **数値のみ変更** 21→27 (η-9/η-10/η-11 範式継承)。

### §3.2 設計範式継承: η-11 §3.4 cascade pair shift 順方向 ACCEPT 判定範式 第 2 例

**概要**: η-11 で確立された cascade pair shift 順方向 既達退行 ACCEPT 判定範式を **第 2 例として直接適用**。判定 matrix の **Y = X 境界条件** で AYA judgment 仰ぐ範式追加。

**η-12 適用**:

| Detection step | 結果 | 判定 |
|---|---|---|
| 1. Y count (既達退行) | **4** (undeclared 1 + nameless 2 + non-opaque 増分 1) | - |
| 2. X count (主指標解消) | **4** (overlapping=21 解消) | - |
| 3. 退行 program = patch program | ✓ **完全一致** (Underwater + EnvMap + WLSky + WLCloud) | - |
| 4. 退行 root cause = cascade | ✓ parse depth-drill (各 program は依然 parse failed = link 失敗 program 数 ±0) | - |
| **総合判定** | **Y = X = 4 境界条件** | **AYA judgment 仰ぎ「ACCEPT して η-13 移管」確定 2026-06-02** |

**判定 matrix 拡張** (η-11 §3.4 から):

| Y vs X | 退行出自 = patch program | 退行出自 ≠ patch program | 判定 |
|---|---|---|---|
| Y > X | (any) | (any) | **REVERT** (η-9 §3.2 範式) |
| Y < X | ✓ patch program | (n/a) | **ACCEPT** (cascade pair shift 順方向、η-11 §3.4 範式) |
| Y = X | ✓ patch program | (n/a) | **AYA judgment 仰ぎ** (本範式新規、境界条件) |
| Y < X | n/a | ✓ 別 program | **要 review** (root cause 不明、AYA 判断仰ぐ) |

**handoff doc 記録価値**: 後続 sub-bundle で Y = X 境界条件発生時、AYA judgment を仰ぐ範式として明確化 = 判定矩形化 + AYA 判断仰ぐ範囲を Y = X + Y > X (一律 REVERT) + 出自不一致 の 3 ケースに整理。

### §3.3 設計範式継承: η-11 §3.5 Agent 報告検証 step V/F pair 同定範式違反検出範式 (η-12 では未適用 = 直接同定で済)

**概要**: η-11 で確立された Agent 報告検証 step 範式は η-12 では未適用 = 着手前 trace で V/F pair を **直接 grep 同定** したため Agent 投入なしで V/F 同期 patch を直接計画。

**η-12 適用外の理由**:
- η-12 patch 対象 = η-11 patch 7 V file の subset (3 V file)
- loc=21 を持つ V file は waterV/skyV/cloudsV の 3 件のみ (terrainV/sunDiscV/SMAABlendWeightsV/screenSpaceReflPostV は loc=21 持たず)
- → 着手前 trace で全 7 V file 直接 grep + F partner 同定で 7 file 確定 = Agent 投入不要

**範式継承**: η-11 §3.5 範式は **Agent 投入時** に必須適用、本 sub-bundle のように **少数 file 直接 grep** で同定可能な場合は不要 = 範式適用条件の明示化。

### §3.4 新規 設計範式: handoff doc canonical 記載 着手前再検証範式 記録漏れ補完範式

**概要**: η-10 §3.1 / η-11 §3.2 で確立された handoff doc canonical 記載 着手前再検証範式の **記録漏れ補完範式** として新規確立。前 sub-bundle handoff doc の §2 metric 表 + §10.1 推奨 scope の **literal grep 再検証** で記録漏れを発見 → 当該 sub-bundle scope に追加可能。

**位置付け**: feedback_doubt_self_first 範式の **handoff doc に対する適用**。handoff doc は完遂時 snapshot で記録されるが、verify cycle の log を全件 literal grep し直さない限り **emerging metric の漏れ** が発生する。着手前 trace step 4 (handoff canonical 記載再検証) で literal grep を実行することで漏れを発見可能。

**Detection 手順** (4 step):
1. **handoff doc §2 metric 表抽出**: 前 sub-bundle 末 metric 件数を全件抽出
2. **literal grep 再検証**: 現 log で **全 metric の literal grep** 再実行 (η-12 では 16 metric)
3. **齟齬検出**: handoff doc 記録 vs 現 log 一致確認、不一致は記録漏れ可能性
4. **新 metric 系統発見**: handoff doc に **未記録** の error 系統 (e.g. NEW overlapping=21 location 値) を発見した場合、当該 sub-bundle scope に追加候補化

**η-12 適用例**:
- η-11 handoff §2: `overlapping use of location 20` 11→0 と記載、`overlapping use of location 21` 等の **異 location 値の overlapping** は記録なし
- η-12 着手前 literal grep: `overlapping use of location` (全件) = 4 件発見 = NEW location=21 4 件
- → handoff doc §10.1 推奨 scope 4 候補 (A non-opaque / B SPIR-V / C normalMap / D depthMap) に **NEW B' (overlapping=21 4 件)** 追加候補化
- AYA「B' 単独で進めて」judgment 確定 = 記録漏れ補完範式 適用効果実証

**handoff doc 記録価値**: 後続 sub-bundle で handoff doc 末記録漏れ発見 → 当該 sub-bundle scope に組み込む範式 = handoff doc の **完璧性に依存しない trace 範式**、verify cycle log を independent source として扱う = feedback_doubt_self_first 範式の handoff doc 適用形。

### §3.5 設計範式継承表 (η-1〜η-11 全件継承)

| η-N | 範式 | η-12 適用状況 |
|---|---|---|
| η-1 §3.1 | FrameViewProj guard wrap (set=0/binding=0) | η-12 適用外 (location qualifier 系) |
| η-1 §3.2 | cascade pair hypothesis (主指標 -X ↔ cascade +Y) | η-12 §3.2 で Y = X 境界条件範式拡張 |
| η-1 §3.3 | Agent 並列 disjoke scope | η-12 適用外 (Agent 投入なし、直接 grep 同定) |
| η-2 (a) §3.2 | UBO body member 順序 std140 完全一致 | η-12 適用外 |
| η-3 §3.1/§3.2 | Link failed root cause UBO body 差異解消 | η-12 適用外 (Link failed 0 維持) |
| η-4 §3.1/§3.2/§3.4 | nameless block × padding / cascade pair / UBO set=3 帯 | η-12 退行 nameless block 2 件は η-4 §3.1 系統 = η-13 で同範式適用候補 |
| η-5 §3.1/§3.2/§3.3/§3.4 | multi-root-cause 同時対処 / 5th-level scope refinement / Agent depth trace / WEIGHT_LOCATION_DEFINED guard | η-12 適用外 (Phase 単一 root cause = overlapping=21 のみ) |
| η-6 §3.1/§3.2/§3.3/§3.4/§3.5 | multi-cluster pilot / Agent 誤判定 → re-trace / nameless block × member / Agent capability limit 移管 / cascade pair 逆方向 | η-12 適用外 (Agent 投入なし) |
| η-7 §3.1/§3.2/§3.3/§3.4/§3.5 | bvec2→uvec2 / program name collision / 5th-level cascade emergence / nameless block × member 第 4 例 / cascade pair 順方向 | η-7 §3.5 cascade pair 順方向は η-11 §3.4 + 本 sub-bundle §3.2 の前駆 |
| η-8 §3.1/§3.2/§3.3/§3.4 | cinematic_bd override path / runtime preprocessed dump / mIndexedTextureChannels Vulkan-aware emit / UBO wrap 範式 | η-12 では runtime dump 未投入 (static trace で V/F + helper 同定可能) |
| η-9 §3.1 | V/F pair canonical partner 同定範式 | η-12 §3.1 で **直接適用第 4 例** |
| η-9 §3.2 | Phase 1 falsification + 完全 revert + Phase 2 軌道修正 | η-12 では Phase 1 即 ACCEPT (1-shot 完結) |
| η-9 §3.3 | feedback_falsification_as_progress sub-bundle 内 phase falsification | η-12 §3.2 で Y = X 境界条件 ACCEPT として適用 |
| η-9 §3.4 | feedback_admit_unknown Phase 1 → Phase 2 切替 | η-12 適用外 (1-shot 完結) |
| η-10 §3.1 | handoff doc canonical 記載 着手前再検証範式 | η-12 §3.4 で **記録漏れ補完範式** として拡張 |
| η-10 §3.2 | V/F pair canonical partner 同定範式 第 2 例 | η-12 §3.1 で第 4 例継承 |
| η-10 §3.3 | 1-shot ACCEPT vs 2-phase 構成判定範式 | η-12 で 1-shot ACCEPT 適用第 3 例 |
| η-11 §3.1 | V/F pair canonical partner 同定範式 第 3 例 (auto-attach helper 経路 canonical 同定範式拡張) | η-12 §3.1 で第 4 例 (atmosphericsVarsV 第 2 var vary_AtmosAttenuation loc=21 適用) |
| η-11 §3.2 | handoff doc canonical 記載 着手前再検証範式 第 2 例 | η-12 §3.4 で **記録漏れ補完範式** として拡張 |
| η-11 §3.3 | 1-shot ACCEPT vs 2-phase 構成判定範式 第 2 例 | η-12 で第 3 例継承 |
| η-11 §3.4 | cascade pair shift 順方向 既達退行 ACCEPT 判定範式 | η-12 §3.2 で **直接適用第 2 例** + Y = X 境界条件範式拡張 |
| η-11 §3.5 | Agent 報告検証 step V/F pair 同定範式違反検出範式 | η-12 適用外 (Agent 投入なし) + 範式適用条件明示化 |

**feedback memory 10 件全件適用確認**:
- feedback_doubt_self_first: η-12 §3.4 適用 (handoff doc §2 + §10.1 + AYA メッセージ scope 候補 B「Layout 残 3」を着手前 literal grep で再検証 → NEW B' 発見)
- feedback_root_cause_not_dump: 3 V + 4 F = 7 file V/F 同期 patch で V 規約 + F 規約 両 side 整合 (canonical atmosphericsVarsV 触らず)
- feedback_shader_only_fast_iterate: shader cp + cache clear のみ (autobuild 不要、C++ touch 0)
- feedback_no_auto_commit: AYA「commit して」明示指示後に commit
- feedback_no_claude_coauthor: commit message に Co-Authored-By 不在
- feedback_one_step_at_a_time: 着手前 trace → scope 候補比較提示 → AYA judgment 仰ぎ → patch → deploy → verify → AYA judgment 仰ぎ → handoff doc 起草 を順次実施
- feedback_proactive_handoff: 本 handoff doc 起草
- feedback_no_scope_shrink: η-12 scope (overlapping=21 4 件のみ) は AYA 明示指示「B' 単独で進めて」= scope shrink でなく AYA 確定 scope
- feedback_admit_unknown: η-12 では仮説 2 連続外れず (handoff doc §10.1 漏れ発見 → 即 §3.4 範式確立、AYA judgment 仰ぎで Y = X 境界条件確定)
- feedback_falsification_as_progress: η-12 §3.2 で Y = X 境界条件 ACCEPT 判定として適用 (各 program は parse failed 状態維持 = link 失敗 program 数 ±0 = net 進展)

---

## §4 cold cache launch verify 詳細

### §4.1 verify cycle 構成 (1 cycle = 1-shot ACCEPT)

**Cycle 1 (Phase 1 verify = ACCEPT)**:
- 操作: 7 file patch → shader-only fast-iterate cp 7 file → shader_cache clear (0 件 verify) → AYA cold cache launch (22:50:36 - 22:51:15、約 40 秒) → shutdown
- 結果:
  - `overlapping use of location 21` 4 → **0** (-4 ✓ 100% 完全達成)
  - `undeclared identifier` 0 → 1 ⚠️ (WLCloud cloud_scale)
  - `nameless block ... global scope` 0 → 2 ⚠️ (EnvMap + WLSky FrameAtmosphere_Skybox)
  - `non-opaque uniforms outside a block` 5 → 6 ⚠️ (+1、Underwater)
  - 計 Y = 4、X = 4、**Y = X 境界条件**
  - 9 種既達主指標 0 完全維持 + normalMap 69 / depthMap 9 維持
  - 第10層 emergence 残系統 (SPIR-V 41) 完全 ±0 (η-13 移管継続)
  - clean shutdown (Goodbye 1 + Vulkan device destroyed 1 + FATAL/SIGSEGV/Aborted 0)
- 判定: **η-12 deliverable 確定 (1-shot ACCEPT、AYA judgment 確定 2026-06-02)**

### §4.2 主指標 metric integrity self-check

- `overlapping use of location 21` -4 = 3 V file + 4 F file の location=21 → 27 整合直接効果 (4 program 全件、各 program の vertex/fragment stage で atmosphericsVarsV.glsl:33 vary_AtmosAttenuation (loc=21) と main shader file の out/in (loc=21) 衝突解消)
- `'location'` literal grep 0→0 (Layout match 0 + overlapping 0 + SPIR-V 41 は literal 'location' 経由なしの error category)
- parse failed 130→130 (cascade pair shift 内訳のみ、parse failed program 数は同等維持 = 各 program 内 root cause depth-drill)

### §4.3 既達退行 (4 件) root cause 同定

| # | metric | program | stage | error LINE | 推定 root cause |
|---|---|---|---|---|---|
| 1 | undeclared identifier 'cloud_scale' | Deferred Windlight Cloud Program | vertex 0x8b30 | 0:1283 | UBO `CloudsVParamUBO_Legacy { ... float cloud_scale ... }` の declaration が nameless block error で parse 失敗 → cloud_scale が undeclared 連鎖 (NEW root cause systems = UBO body member 衝突連鎖) |
| 2 | nameless block ... global scope 'FrameAtmosphere_Skybox' | Environment Map Program | fragment 0x8b31 | 0:1156 | helper auto-attach (`atmosphericsVarsF.glsl`) 内 `FrameAtmosphere_Skybox` UBO member (sunlight_color 等) が main shader file `skyF.glsl` 内 global scope と衝突 (η-4 §3.1 nameless block × padding 範式同系統) |
| 3 | nameless block ... global scope 'FrameAtmosphere_Skybox' | Deferred Windlight Sky Shader | fragment 0x8b31 | 0:1155 | 同上 (skyF.glsl 経由) |
| 4 | non-opaque uniforms outside a block | Underwater Shader | fragment 0x8b31 | 0:1099 | underWaterF.glsl 内 bare uniform 露出 (cascade pair shift 順方向、η-11 §3.4 範式) |

全 4 件が η-12 で patch した program と完全一致 = η-11 §3.4 範式 cascade pair shift 順方向 確定。各 program は **parse failed 状態維持** = link 失敗 program 数 ±0 = 描画上同状態 = net 進展。

### §4.4 shutdown clean verify

- Cycle 1 で:
  - `Goodbye!` 1 件 + `Vulkan device destroyed` 1 件
  - FATAL/SIGSEGV/Aborted 0/0/0

---

## §5 self-verify 18 項目 all green

| # | 項目 | 状態 |
|---|---|---|
| 1 | charter §3 #1 GL path byte-for-byte 不変担保 (7 file GL `#else` path 元宣言不変) | ✓ |
| 2 | shader file のみ touch (C++ touch 0) | ✓ |
| 3 | 主指標 `overlapping use of location 21` -4 literal grep verify | ✓ |
| 4 | 既達主指標 12 種 literal grep verify 全 0 維持 + normalMap/depthMap 69/9 維持 (退行 3 系統計 4 件 AYA「ACCEPT して η-13 移管」judgment 確定) | ✓ |
| 5 | parse failed integrity (130→130 ±0、cascade pair shift 内訳のみ) | ✓ |
| 6 | FATAL/SIGSEGV/Aborted 0 維持 | ✓ |
| 7 | clean shutdown (Goodbye 1 + Vulkan destroy 1) | ✓ |
| 8 | V/F pair canonical partner 同定範式 §3.1 直接適用第 4 例 (auto-attach helper 第 2 var vary_AtmosAttenuation loc=21 適用) | ✓ |
| 9 | cascade pair shift 順方向 既達退行 ACCEPT 判定範式 §3.2 直接適用第 2 例 + Y = X 境界条件範式拡張 | ✓ |
| 10 | handoff doc canonical 記載 着手前再検証範式 §3.4 記録漏れ補完範式新規確立 (η-11 handoff §2 + §10.1 + AYA メッセージ scope 候補 B「Layout 残 3」記録漏れ → NEW B' 発見) | ✓ |
| 11 | Agent 報告検証 step V/F pair 同定範式違反検出範式 §3.3 適用条件明示化 (η-12 では未適用 = 直接 grep 同定で済) | ✓ |
| 12 | η-9 §3.3 falsification 履歴遵守 (canonical atmosphericsVarsV.glsl:33 触らず、loc=21 既使用 audit で safe loc=27 選定) | ✓ |
| 13 | 過去 sub-bundle 既処理 file 第 2 touch (η-11 で 15 file 全件初 touch、η-12 では subset 7 file 第 2 touch = B?-δ 範式 admission) | ✓ |
| 14 | skip list admission 範式継承 (7 file は η-11 で初 touch 済、η-12 第 2 touch も skip list 触れず) | ✓ |
| 15 | loc=27 safe location audit 完全実施 (全 shader directory grep 0 件 = 完全未使用 ✓ + waterV/skyV/cloudsV 全 loc 使用 audit で衝突なし確認) | ✓ |
| 16 | feedback_no_claude_coauthor 遵守 (commit message Co-Authored-By 不在) | ✓ |
| 17 | feedback_no_auto_commit 遵守 (AYA「commit して」明示指示後に commit) | ✓ |
| 18 | handoff doc 別 commit (η-11-complete `74705c35bb` 範式継承) | ✓ |

---

## §6 設計範式継承表

(§3.5 と内容重複のため §3.5 参照、feedback memory 10 件全件適用確認 + prior sub-bundle 32+ 件継承 + 新規 1 件 §3.4 + 拡張 1 件 §3.2 Y = X 境界条件)

---

## §7 risks

| # | risk | mitigation |
|---|---|---|
| 1 | Y = X 境界条件 ACCEPT 判定範式 §3.2 の **濫用** (Y > X でも AYA judgment 仰がず ACCEPT 誤適用) | §3.2 判定 matrix 厳守、Y > X は **必ず REVERT** (η-9 §3.2 範式) で固定、Y = X のみ AYA judgment 仰ぎ |
| 2 | handoff doc 記録漏れ補完範式 §3.4 の **適用範囲過大** (全 sub-bundle で着手前 literal grep 全件再実行は工数膨張) | §3.4 適用は **handoff doc §2 metric 表 + §10.1 推奨 scope** の re-grep verify (16 metric 程度) に限定、全 log line 再 grep は不要 |
| 3 | η-9 §3.1 V/F pair canonical partner 同定範式 第 4 例の **canonical helper file 内 複数 var 持つ場合の整合範式不在** | η-13+ で canonical helper 内 **全 var の整合を同 sub-bundle 内で完結** する範式追加検討、η-12 教訓 = η-11 で loc=20 のみ整合させた結果 η-12 で loc=21 cascade 露出 |
| 4 | 既達退行 4 件 (undeclared 1 + nameless 2 + non-opaque 1) **η-13 移管** で残量増 (η-12 末: non-opaque 6 + undeclared 1 + nameless 2 + SPIR-V 41 + normalMap 69 + depthMap 9 = 計 128 件) | η-13 着手前 trace で系統別 Phase 分割継続 (nameless block 2 件 = 1 Phase 最優先、cascade root cause 推定容易) + non-opaque 復活 + SPIR-V 41 等と混在 = sub-bundle 細分化方針継続 |
| 5 | loc=27 を **safe location** として確立した範式 (7 file 全件 27 統一) で **後続 sub-bundle で 27 を他用途で割当不可** = location 番号余り減少 | η-13+ で safe location 番号選定時に **27 既使用** を着手前 trace で明示確認、η-9 §3.3 falsification 履歴 (loc=20 既使用) + η-11 (loc=26 既使用) + η-12 (loc=27 既使用) を都度更新 |
| 6 | nameless block 2 件 (EnvMap + WLSky `FrameAtmosphere_Skybox`) は **helper UBO member × global scope 衝突** = η-4 §3.1 範式系統だが、η-12 patch では loc=21→27 numeric edit のみで誘発 = **根本 root cause = skyF.glsl 内 global scope 宣言 vs helper UBO member 重複** | η-13 着手前 trace で skyF.glsl 内 `sunlight_color` 等 global scope 宣言 + atmosphericsVarsF.glsl + FrameAtmosphere_Skybox UBO member 列挙 → 重複検出 → global scope 宣言 remove or UBO 経由参照に統一 |
| 7 | undeclared 'cloud_scale' (WLCloud) は **UBO body member declaration が nameless block error で parse 失敗** の連鎖 = 真の root cause は nameless block error 連鎖 | η-13 で nameless block 解消すれば undeclared も連鎖解消可能性高 = nameless block 優先処理 |
| 8 | non-opaque +1 (Underwater line 1099) は **parse 進行で次フェーズ露出** = 元々 hidden bug = η-12 patch 自体は loc=21→27 numeric edit のみで誘発、根本対処は underWaterF.glsl 内 bare uniform → UBO wrap (η-8 §3.4 範式) | η-13 で non-opaque 系統対処時に Underwater 含む 6 件全件まとめて UBO wrap 処理 |
| 9 | charter §3 #1 担保が **既存 numerical literal 変更** (loc 番号 21→27、7 file) で揺らぐ懸念 | η-9/η-10/η-11 §3.2 範式既確立 = GL `#else` path は数値含め完全不変、Vulkan path 内 layout(location=N) 数値のみ変更 = 既範式の自然延長と整理、η-12 で 7 file 一括適用で範式継続実証 |
| 10 | shader file のみ touch 範式の **runtime emit (C++) 系 root cause 取りこぼし** = η-8 §3.3 で 1 例発生、η-9/η-10/η-11/η-12 で 0 だが η-13 SPIR-V 41 件対処時に再発可能性高 | η-13 着手前に **C++ shader 関連 file (llshadermgr.cpp / llglslshader.cpp / llviewershadermgr.cpp) の bare uniform / sampler emit 系列挙** を grep で先回り (η-11 §7 #9 継承) |
| 11 | cinematic_bd directory 全 .glsl audit (§10.4 = η-8 から継承、η-9・η-10・η-11・η-12 未実施) **未実施** | η-13 着手前 or 完遂後の別 phase として強く推奨 (η-8 §3.1 cinematic_bd override path 発見範式の予防的展開、η-11 §7 #10 継承) |
| 12 | shader_cache 件数 (η-7 305 baseline → η-11 310) **η-12 計測未実施** | η-13 で shader_cache 件数計測再開 + 第12層 emergence 観測 |
| 13 | Phase 単一 deliverable が **7 file 1 機能** で η-N sub-bundle scope として中規模 = AYA judgment 「B' 単独で進めて」明示指示下 scope 確定 = scope shrink でなく AYA 確定 scope (η-11 §7 #12 継承) | feedback_no_scope_shrink 違反でなく **AYA 明示指示** = 7 file は V/F pair 同期で **必要十分** の patch 範囲、scope 拡大ではない |
| 14 | feedback_no_scope_shrink 違反懸念 (handoff §10.1 4 候補 (A/B/C/D) + NEW B' 1 件中 1 系統のみ scope 化 + 既達退行 4 件を η-13 移管) | AYA「B' 単独で進めて」明示指示下 scope 確定 + 「ACCEPT して η-13 移管」judgment 確定 (2026-06-02)、残 3 候補 + 既達退行 4 件は η-13+ 移管 = scope shrink でなく **scope 分割 + 副次効果移管** (η-11 §7 #13 継承) |
| 15 | cascade exposure 第10層 残 (SPIR-V 41 + normalMap 69 + depthMap 9 = 計 119 件) + 第11層 emergence (non-opaque 6 + undeclared 1 + nameless 2 = 9 件) = 計 128 件 η-13 移管 = **sub-bundle 残量大** | η-13 着手前 trace で系統別 Phase 分割継続 (nameless block 2 件 = 1 Phase 最優先 + undeclared 1 件は連鎖 cascade、nameless block 解消で連鎖解消可能性) |
| 16 | loc=21 → 27 で **7 file 一気の数値変更** は η-11 (15 file +15/-15) より小規模だが review 容易性確保が課題 | git diff --stat + git diff content grep で全件 `layout(location=21)` → `layout(location=27)` のみ確認、numeric-only edit pattern で review 容易性確保 (η-11 §7 #15 継承) |
| 17 | atmosphericsVarsV/F canonical helper は **loc=20 (vary_AdditiveColor)** + **loc=21 (vary_AtmosAttenuation)** の 2 var を持つ = η-11 で loc=20 のみ整合させた結果 η-12 で loc=21 cascade 露出 | η-13+ で canonical helper 内 **複数 var の整合は同 sub-bundle 内で完結** = §7 #3 mitigation 範式追加検討 |

---

## §8 commit history

| commit | sub-bundle | scope |
|---|---|---|
| (η-12 patch 予定) | B?-η-12 patch | 3 V + 4 F = 7 file Vulkan path location 21 → 27 整合 (overlapping use of location 21 4→0 達成) |
| `74705c35bb` | B?-η-11 handoff doc | (η-11 完遂 handoff、η-12 着手境界引継) |
| `9a271fc900` | B?-η-11 patch | 7 V + 8 F = 15 file Vulkan path location 20 → 26 整合 (overlapping use of location 20 11→0 達成) |
| `9246142639` | B?-η-10 handoff doc | (η-10 完遂 handoff) |
| `e1d5ffd98c` | B?-η-10 patch | diffuseAlphaMaskF.glsl Vulkan path location 4/2/0 → 0/1/2 整合 |
| `6fee4a818b` | B?-η-9 handoff doc | (η-9 完遂 handoff) |
| `50cb5f6713` | B?-η-9 patch | avatarEyesV.glsl Vulkan path location 4/2/0 → 0/1/2 整合 |
| `6994eba271` | B?-η-8 handoff doc | (η-8 完遂 handoff) |
| `2542905ce0` | B?-η-8 patch | non-opaque 1 + 'binding' 25 完全達成 |
| `b1e8689634` | B?-η-7 handoff doc | (η-7 完遂 handoff) |
| `874d1a7252` | B?-η-7 patch | nameless / weight redefinition / non-opaque 67% |

---

## §9 file inventory (η-12 patch 7 file 内訳)

| # | file | 変更内容 | 行数 | 範式適用 |
|---|---|---|---|---|
| 1 | `indra/newview/app_settings/shaders/class1/environment/waterV.glsl` | Vulkan path location 21 → 27 (littleWave) | +1/-1 | §3.1 V/F pair canonical partner 同定範式 第 4 例 (atmosphericsVarsV vary_AtmosAttenuation を canonical として尊重、littleWave 移動) |
| 2 | `indra/newview/app_settings/shaders/class3/environment/waterF.glsl` | Vulkan path location 21 → 27 (littleWave) | +1/-1 | V/F pair 同期 patch (waterV partner) |
| 3 | `indra/newview/app_settings/shaders/class3/environment/underWaterF.glsl` | Vulkan path location 21 → 27 (littleWave) | +1/-1 | V/F pair 同期 patch (waterV 第 2 F partner、Underwater Shader 専用) |
| 4 | `indra/newview/app_settings/shaders/class1/deferred/skyV.glsl` | Vulkan path location 21 → 27 (vary_LightNormPosDot) | +1/-1 | §3.1 V/F pair canonical partner 同定範式 第 4 例 (2 program 共有 = EnvMap + WLSky) |
| 5 | `indra/newview/app_settings/shaders/class1/deferred/skyF.glsl` | Vulkan path location 21 → 27 (vary_LightNormPosDot) | +1/-1 | V/F pair 同期 patch (skyV partner) |
| 6 | `indra/newview/app_settings/shaders/class1/deferred/cloudsV.glsl` | Vulkan path location 21 → 27 (vary_CloudColorAmbient) | +1/-1 | §3.1 V/F pair canonical partner 同定範式 第 4 例 |
| 7 | `indra/newview/app_settings/shaders/class1/deferred/cloudsF.glsl` | Vulkan path location 21 → 27 (vary_CloudColorAmbient) | +1/-1 | V/F pair 同期 patch (cloudsV partner) |

**計**: 7 file +7/-7、shader file のみ、C++ touch 0

**overlap file 確認**:
- 全 7 file: η-11 で初 touch 済 (loc=20→26)、η-12 で第 2 touch (loc=21→27) = B?-δ 範式 admission
- atmosphericsVarsV.glsl / atmosphericsVarsF.glsl: η-11 + η-12 で参照のみ (canonical 同定)、touch 0
- llshadermgr.cpp / llviewershadermgr.cpp: η-12 で参照のみ (auto-attach 経路同定 + program file 列挙)、touch 0

---

## §10 次 sub-bundle B?-η-13 推奨 scope

### §10.1 確定 scope: 既達退行 4 件復活 + 第11層 emergence 残 + 第10層 emergence 残全件

| metric | η-12 末件数 | 推定 root cause | 推奨対処 |
|---|---|---|---|
| **nameless block ... global scope** | **2** | EnvMap + WLSky `FrameAtmosphere_Skybox` UBO member × global scope 衝突 (skyF.glsl 内 sunlight_color 等 global 宣言と helper UBO member 重複、η-4 §3.1 範式系統) | 専用 phase (skyF.glsl global scope 宣言 remove or UBO 経由参照に統一) |
| **undeclared identifier** | **1** | WLCloud 'cloud_scale' = UBO `CloudsVParamUBO_Legacy` declaration が nameless block error で parse 失敗 → cloud_scale undeclared 連鎖 | nameless block 解消で連鎖解消可能性高 = 同 phase 内同時解消 |
| **non-opaque uniforms outside a block** | **6** | underWaterF.glsl line 1099 + terrainF.glsl line 1146 + SMAA Blending Weights V ×4 line 1712 (cascade pair shift 順方向 + 既存 hidden) | 専用 phase (bare uniform 探索 + UBO wrap、η-8 §3.4 範式継承) |
| SPIR-V requires location | 41 | in/out 宣言の location 番号未指定 (mass) | C++ runtime emit Vulkan-aware 化 (η-8 §3.3 範式) or 一括 wrap script |
| 'normalMap' redefinition | 69 | (η-1〜η-12 未対処、新 root cause 系統) | 専用 phase (Agent 並列 disjoint scope η-1 §3.3) |
| 'depthMap' redefinition | 9 | 同上 | 同上 |
| missing #endif / parse failed | cascade 縮小傾向 | cascade pair shift | 主指標解消後の自然減 |

### §10.2 B?-η-13 着手前 trace 範式 9 ステップ (η-12 §3.2/§3.4 範式追加)

1. **literal grep**: `grep -c "nameless block" log` + `grep -c "undeclared identifier" log` + `grep -c "non-opaque uniforms outside a block" log` + `grep -c "SPIR-V requires" log` + `grep -c "'normalMap'" log` + `grep -c "'depthMap'" log` + `grep -c "overlapping use of location" log` (η-12 達成維持 = 0 確認) + `grep -c "Layout location qualifier" log` (η-10 達成維持 = 0 確認)
2. **handoff doc 記録漏れ補完範式** (**η-12 §3.4 新規**): 前 sub-bundle handoff §2 metric 表 + §10.1 推奨 scope の **literal grep 再検証** で記録漏れ発見 → 当該 sub-bundle scope に追加候補化
3. **log context 抽出**: 各 program の error LINE × 件数 + stage type 番号 (0x8b30 vertex / 0x8b31 fragment) を **必ず併記** (η-6 §3.2 範式)
4. **V/F pair 同定** (η-9 §3.1 / η-10 §3.2 / η-11 §3.1 / **η-12 §3.1 第 4 例 拡張**): location 系は **`llviewershadermgr.cpp` で `mFragmentFiles.push_back` + `mVertexFiles.push_back` 同 program 内列挙** で V/F file 確定 + **他 program での共有確認** で canonical partner 同定 + **auto-attach helper 経路 (llshadermgr.cpp `attachVertexObject` / `attachFragmentObject`) も grep で同定** + **canonical helper 内 全 var の整合を同 sub-bundle 内で完結** (§7 #3 mitigation)
5. **handoff doc canonical 記載再検証** (η-10 §3.1 / η-11 §3.2 / **η-12 §3.4 拡張**): 前 sub-bundle (η-12) の canonical 同定記録を **着手時 grep で再確認** = canonical 判定が context 依存であることを前提、loc=27 既使用 audit 必須
6. **Agent 並列 disjoint scope** (η-1 §3.3 範式): metric 系統別に **root cause 軸** で分割 (nameless / undeclared / non-opaque / SPIR-V missing / normalMap / depthMap 各別 Agent)
7. **Agent 報告検証 step** (η-6 §3.2 / η-11 §3.5 範式 V/F pair 同定範式違反検出): stage type / source file 同定の literal grep 検証 + **location qualifier 系は V/F pair partner 全件確認** を patch 化前に必須実施 (η-12 では Agent 投入なしで適用外、η-13 Agent 投入時に適用)
8. **1-shot ACCEPT vs 2-phase 構成判定** (η-10 §3.3 / η-11 §3.3 / **η-12 §3.3 適用条件明示化**): 着手前 trace で canonical 確定なら 1-shot、不確定なら 2-phase 構成で Phase 1 falsification 許容
9. **cascade pair shift 既達退行 ACCEPT 判定** (**η-11 §3.4 / η-12 §3.2 Y = X 境界条件範式拡張**): verify 後に既達退行検出時、Y < X + 退行出自 = patch program 確認で ACCEPT、Y > X または 退行出自 ≠ patch program は AYA 判断仰ぐ、**Y = X 境界条件は AYA judgment 必須**

### §10.3 B?-η-13 完遂後の想定 cascade exposure 第13層

- nameless block 2 件解消想定 = undeclared 連鎖解消 (cloud_scale) 想定 = 計 -3 程度
- non-opaque 6 件解消想定 = parse failed -6 程度
- SPIR-V missing 41 件 → C++ runtime emit Vulkan-aware 化で大量解消想定
- normalMap / depthMap redefinition 78 件 → 別 sub-bundle 分割推奨 (B?-η-14 移管候補)
- shader_cache 件数: η-13 で 310+ 維持 + 第13層 emergence 観測

### §10.4 cinematic_bd 系統 systematic audit 推奨 (η-8 から継承、η-9・η-10・η-11・η-12 未実施)

η-8 §3.1 cinematic_bd override path 発見範式で shadowUtil のみ個別検出。cinematic_bd directory 内 .glsl file 全件で類似 override + 未 wrap bare uniform / 未整合 location 残存可能性。B?-η-13 着手前 or 完遂後の別 phase として:

1. `find indra/newview/app_settings/shaders/cinematic_bd -name "*.glsl"` で全件列挙
2. 各 file の bare uniform / sampler / `layout` 欠落 + location qualifier 不整合 grep audit
3. 該当 file を class1/class2/class3 版 patch と同 set/binding + location で wrap / 整合 (mutually exclusive 担保)

η-8 で発生した「想定外 cinematic_bd 起源 metric 露出」と η-11/η-12 で発生した cascade pair shift 順方向 (非主指標退行) の **両方を予防的に解消**する。

### §10.5 runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画継承

η-9 / η-10 / η-11 / η-12 では未投入 (static trace で V/F + auto-attach helper 同定可能だった)。η-13 で nameless block 2 件 (skyF.glsl FrameAtmosphere_Skybox helper UBO member × global scope 衝突) root cause が helper concat 経由で static trace 不能な場合、η-8 §3.2 範式再投入 = `llglslshader.cpp` parse-fail 路径に LL_WARNS diag dump 一時追加 → sub-bundle 完遂時に **必ず除去** (η-12 baseline 完全復元)。

### §10.6 location safe number audit 範式 (η-11 §3.1 第 3 例 + η-12 §3.1 第 4 例で確立)

η-11 で 15 file 全件 loc=26 統一 + η-12 で 7 file 全件 loc=27 統一 = 後続 sub-bundle で loc=26 / loc=27 を **他用途で割当不可** に変化。B?-η-13+ で **safe location 番号選定時の必須 audit step**:

1. `grep -rn "layout(location=N)" indra/newview/app_settings/shaders/` で全件列挙
2. 各 N の使用 file + program 同定
3. **完全未使用 N** または **本 sub-bundle 対象 program と重複しない N** を safe location 候補化
4. η-9 §3.3 falsification 履歴 (loc=20 既使用) + η-11 §3.1 (loc=26 既使用) + η-12 §3.1 (loc=27 既使用) を都度更新

### §10.7 canonical helper file 内 複数 var 整合範式 (η-12 §7 #3 mitigation で提示、η-13+ で範式化検討)

η-12 教訓: atmosphericsVarsV/F canonical helper は loc=20 (vary_AdditiveColor) + loc=21 (vary_AtmosAttenuation) の 2 var を持つ = η-11 で loc=20 のみ整合させた結果 η-12 で loc=21 cascade 露出。

η-13+ で canonical helper file を patch 対象とする際は:
1. **canonical helper 内 全 var の grep 列挙**
2. **各 var の loc 番号と patch 対象 main shader 側の loc 番号 整合確認**
3. **同 sub-bundle 内で全 var 整合完結** (1 var のみ整合 → 別 var cascade 露出予防)

---

## §11 観測点

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-11 310) | η-12 計測未実施 | B?-η-13 で計測再開 + 第13層 emergence 観測 |
| 2 | V/F pair canonical partner 同定範式 第 4 例 (§3.1) 適用 | 適用済 (7 file V/F 同期、auto-attach helper 第 2 var vary_AtmosAttenuation loc=21 適用) | B?-η-13 でも location qualifier 系で同範式継続適用 + canonical helper 内 全 var 整合 (§10.7) |
| 3 | cascade pair shift 順方向 既達退行 ACCEPT 判定範式 (§3.2) 直接適用第 2 例 + Y = X 境界条件範式拡張 | 適用済 (Y=X=4、退行出自 = patch program、AYA「ACCEPT して η-13 移管」judgment 確定) | B?-η-13 で既達退行検出時に Detection 手順 4 step 適用、Y = X 境界条件は AYA judgment 必須 |
| 4 | handoff doc 記録漏れ補完範式 (§3.4) 新規確立 | 適用済 (η-11 handoff §2 + §10.1 + AYA メッセージ scope 候補 B「Layout 残 3」記録漏れ → NEW B' 発見) | B?-η-13 着手前 trace step 2 として継続適用 |
| 5 | 1-shot ACCEPT vs 2-phase 構成判定範式 (§3.3) 適用条件明示化 | 適用済 (1-shot、Agent 投入なし) | B?-η-13 で着手前 trace 確定度で判定 |
| 6 | Agent 報告検証 step V/F pair 同定範式違反検出範式 (η-11 §3.5) 適用条件明示化 | η-12 では未適用 (直接 grep 同定で済) | B?-η-13 で Agent 投入時に適用 |
| 7 | set/binding allocation 連続割当 (η-8 末: 14-59 + 100-103) | η-12 で binding touch なし | B?-η-13 で SPIR-V missing 41 件対処時に binding 触る場合は 60+ 連続割当開始 |
| 8 | C++ touch 0 維持 (η-8 で 1 例発生、η-9/η-10/η-11/η-12 で 0) | η-12 達成 | B?-η-13 で SPIR-V missing 41 件対処時に C++ touch 復活可能性 (η-8 §3.3 範式) |
| 9 | 既達主指標完全維持 (η-12 で 12 種、退行 3 系統 4 件 η-13 移管) | △ η-12 で 3 系統 4 件退行 AYA judgment で移管 | B?-η-13 で退行 4 件復活 + 12 種維持 + 残系統 scope |
| 10 | cinematic_bd directory 全 .glsl audit (§10.4) | 未実施 (η-8 から継承、η-9/η-10/η-11/η-12 全て未実施) | B?-η-13 着手前 or 完遂後の別 phase として推奨 |
| 11 | runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画 | η-9/η-10/η-11/η-12 未投入 | B?-η-13 で nameless block 2 件 (FrameAtmosphere_Skybox helper UBO member × global scope 衝突) root cause が static trace 不能な場合に再投入 + sub-bundle 完遂時除去 |
| 12 | location safe number audit (§10.6) | η-12 で loc=27 既使用追加 (η-11 loc=26 + η-9 履歴 loc=20) | B?-η-13+ で safe location 番号選定時に必須 audit step |
| 13 | canonical helper file 内 複数 var 整合範式 (§10.7) | η-12 教訓抽出 (atmosphericsVarsV/F loc=20 + loc=21 の 2 var) | B?-η-13+ で canonical helper 内 全 var 整合を同 sub-bundle 内で完結 |
| 14 | cascade exposure 第10層 残 (SPIR-V 41 + normalMap 69 + depthMap 9 = 119 件) + 第11層 emergence (non-opaque 6 + undeclared 1 + nameless 2 = 9 件) = 計 128 件 η-13 移管 | sub-bundle 残量大 | B?-η-13 着手前 trace で系統別 Phase 分割継続推奨 (nameless block 2 件 = 1 Phase 最優先、undeclared 連鎖解消可能性高) |

---

**handoff doc 完。次 sub-bundle B?-η-13 着手は本 doc §10 推奨 scope を起点として、fresh context で実施。**
