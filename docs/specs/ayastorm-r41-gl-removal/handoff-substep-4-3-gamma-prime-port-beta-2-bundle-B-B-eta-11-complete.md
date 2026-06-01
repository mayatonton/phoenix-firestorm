# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-11 完遂 handoff

**status**: B?-η-11 完遂 → 次 sub-bundle B?-η-12 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: (η-10 patch `e1d5ffd98c` 範式継承、AYA 「commit して」明示指示下 commit 予定 2026-06-02)
**handoff doc commit**: 本 doc (η-10-complete `9246142639` 範式継承、別 commit 予定)
**勤続範式継承**: B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-11 scope = η-10 handoff §10.1 確定 **第10層 emergence `overlapping use of location 20` 11 件解消** (Phase 単一、AYA scope A 確定で系統別 Phase 分割の 4 候補 (overlapping / SPIR-V / normalMap / depthMap) のうち overlapping のみ scope)。

- **Phase 単一 (1-shot ACCEPT)**: location=20 重複の **7 V file + 8 F file = 15 file 同期** 整合 = `layout(location=20)` → `layout(location=26)` numeric-only
  - 11 program は全て `mFeatures.isDeferred=true` で `indra/llrender/llshadermgr.cpp:99-101` から `atmosphericsVarsV.glsl` (location=20 `vary_AdditiveColor`) が auto-attach されて衝突
  - canonical = `atmosphericsVarsV.glsl:28` (~25 shader 共有、η-9 §3.3 falsification 履歴整合)
  - patch 対象 = 7 V file + 8 F file (V/F pair 同期、Agent 報告 V-only 提案を grep verify で F partner 追加同定して修正)
  - η-9 §3.1 V/F pair canonical partner 同定範式 **直接適用第 3 例**

**計**: **15 file +15/-15** (shader file のみ、C++ touch 0)

**主指標達成** (vs η-10 baseline):
- `overlapping use of location 20` 11 → **0** ✓ **-11 / 100% 完全達成**

**既達主指標 1 件退行検出** (cascade pair shift 順方向、η-1 §3.2 範式典型例):
- `non-opaque uniforms outside a block` 0 → **5** ⚠️ (Deferred Terrain F + SMAA Blending Weights V ×4)
- η-9 §3.2 範式判定 = Y=5 < X=11 (Y=既達退行 / X=主指標解消) **revert 基準下回り、ACCEPT 範囲**
- AYA 判断「A で」(2026-06-02) で **ACCEPT 維持 + B?-η-12 scope に移管** 確定

**他既達主指標完全維持** (12 種): `Layout location qualifier must match` 0 / `'binding'` 0 / `Cannot reuse block name` 0 / `'weight4' redefinition` 0 / `'weight' redefinition` 0 / `undeclared identifier` 0 / `Link failed` 0 / `'size' undeclared` 0 / `GBufferInfo redefinition struct` 0 / `'#'` 0 / `nameless block ... global scope` 0 + (η-10 完遂時 13 種 0 完全継承 − 1 退行 non-opaque)

**Phase 構成の特徴**: η-10 と同様 **Phase 1 即 ACCEPT の 1-shot 完結**。η-9 §3.1 範式適用で canonical partner を着手前 trace で確定 → falsification 不要で直接 deliverable。ただし Agent 報告 V-only 提案を grep verify で V/F pair 同期 patch に修正した点が η-10 と差別化 = §3.5 範式新規。

---

## §2 完遂結果 metric (vs B?-η-10 baseline commit `e1d5ffd98c`)

| metric | η-10 baseline | η-11 (verify) | Δ vs η-10 | 判定 |
|---|---|---|---|---|
| overlapping use of location 20 | 11 | **0** | -11 | ✓ **100% 完全達成** |
| **non-opaque uniforms outside a block** | **0** | **5** | **+5** | ⚠️ **既達退行** (Deferred Terrain F + SMAA Blending Weights V ×4、cascade pair shift 順方向、η-9 範式 Y<X で ACCEPT) |
| Layout location qualifier must match | 0 | 0 | ±0 | ✓ η-10 達成維持 |
| 'binding' | 0 | 0 | ±0 | ✓ η-8 達成維持 |
| Cannot reuse block name | 0 | 0 | ±0 | ✓ η-1 達成維持 |
| 'weight4' redefinition | 0 | 0 | ±0 | ✓ η-5 達成維持 |
| 'weight' redefinition | 0 | 0 | ±0 | ✓ η-7 達成維持 |
| undeclared identifier | 0 | 0 | ±0 | ✓ η-5 達成維持 |
| Link failed | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| 'size' undeclared | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| '#' preprocessor | 0 | 0 | ±0 | ✓ ε 達成維持 |
| nameless block ... global scope | 0 | 0 | ±0 | ✓ η-7 達成維持 |
| SPIR-V requires location | 41 | 41 | ±0 | ✓ (η-12 移管継続) |
| 'normalMap' redefinition | 69 | 69 | ±0 | ✓ (η-12+ 移管継続) |
| 'depthMap' redefinition | 9 | 9 | ±0 | ✓ (η-12+ 移管継続) |
| 'location' literal grep | 52 | 45 | -7 | (Layout 0 + overlapping -11 + SPIR-V 41 同居で literal 数減、cascade pair shift 内訳) |
| parse failed | 132 | 130 | -2 | (overlapping 解消の cascade pair shift 順方向) |
| shader_cache 件数 | 308 | 310 | +2 | (link 成立 program 集合シフト、η-7 305 baseline + 5) |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | ±0 | ✓ |
| Goodbye | 1 | 1 | ±0 | ✓ clean shutdown |
| Vulkan device destroyed | 1 | 1 | ±0 | ✓ clean shutdown |

**主指標 overlapping use of location 20 100% 完全達成 + 12 種既達主指標完全維持 + non-opaque 既達 1 件退行 AYA 「A で」判断で ACCEPT 移管** = η-11 主 scope 完全達成、退行 non-opaque 5 件 + 残 cascade exposure (SPIR-V 41 + normalMap 69 + depthMap 9) は **B?-η-12 移管**。

---

## §3 設計範式

### §3.1 設計範式継承: η-9 §3.1 V/F pair canonical partner 同定範式 第 3 例

**概要**: η-9 で確立された V/F pair canonical partner 同定範式を **第 3 例として直接適用**。範式自体に変更なし、適用 file 群と program 群のみ拡大:

| 適用例 | 適用 sub-bundle | program 群 | canonical (変更不可) | patch 対象 (1 program 専用または狭範囲) | 結果 |
|---|---|---|---|---|---|
| 第 1 例 | η-9 Phase 2 | gDeferredAvatarEyesProgram | diffuseF.glsl (avatarEyesV と pair 時) | avatarEyesV.glsl (4/2/0 → 0/1/2) | Layout 12→3 (-9) |
| 第 2 例 | η-10 | gDeferredNonIndexedDiffuseAlphaMaskProgram | diffuseV.glsl (3 program 共有) | diffuseAlphaMaskF.glsl (4/2/0 → 0/1/2) | Layout 3→0 (-3) |
| **第 3 例** | **η-11** | **11 isDeferred program 群 (Underwater + Terrain + SMAA ×4 + EnvMap + WLSky + WLCloud + WLSun + SSR Post)** | **atmosphericsVarsV.glsl:28 vary_AdditiveColor (~25 shader 共有、auto-attach 経由)** | **7 V file + 8 F file = 15 file (location=20 → 26 数値変更)** | **overlapping 11→0 (-11)** |

**第 3 例の特徴**: canonical partner が **auto-attach helper file** (llshadermgr.cpp L99-101 経由) で main shader file から見て暗黙的 = grep 単独では認識困難、`indra/llrender/llshadermgr.cpp` の `attachVertexObject("windlight/atmosphericsVarsV.glsl")` 等の attach 経路を読まないと「~25 shader 共有」が見えない。

**charter §3 #1 担保**: 全 15 file の GL `#else` path `out/in <type> <var>;` 元宣言不変、Vulkan path 内 `layout(location=N)` の **数値のみ変更** 20→26 (η-9/η-10 範式継承)。

### §3.2 設計範式継承: η-10 §3.1 handoff doc canonical 記載 着手前再検証範式 第 2 例

**概要**: η-10 で確立された handoff doc canonical 記載 着手前再検証範式を **第 2 例として直接適用**。

**η-11 適用**:
- 前 sub-bundle (η-10) handoff §3.2 = 「diffuseV.glsl 3 program 共有 = 真の canonical」と記載
- η-11 着手時 grep 結果:
  - `atmosphericsVarsV.glsl`: llshadermgr.cpp で **auto-attach 経由 ~25 shader 共有** = **真の canonical** (η-9 §3.3 falsification 履歴整合)
  - 7 V file (waterV/terrainV/skyV/cloudsV/sunDiscV/SMAABlendWeightsV/screenSpaceReflPostV): 各 1-4 program 専用 = **patch 対象**
- → η-10 handoff の「canonical 判定は program context 依存」は本 sub-bundle でも有効、η-11 では「auto-attach helper の context」が canonical 判定の決定要因 = **canonical 判定範囲が attach 経路まで拡大**

### §3.3 設計範式継承: 1-shot ACCEPT vs 2-phase 構成判定範式 (η-10 §3.3) 第 2 例

**概要**: η-10 と同じく **着手前 trace で canonical 確定 → 1-shot ACCEPT** 判定。

**η-11 適用**:
- 着手前 trace step 3 (V/F pair 同定) で:
  - `grep "atmosphericsVarsV.glsl" llshadermgr.cpp` → auto-attach 経路 (L101) 確定
  - `grep "layout(location=20)" indra/newview/app_settings/shaders/ -r` → 全件列挙 60+ で重複箇所同定
  - 各 patch 対象 V/F pair の program 列挙数で「狭範囲 (1-4 program)」確定
- → 1-shot ACCEPT 判定、Phase 1 で直接 deliverable

**ただし η-10 との差異**: Agent 報告が **V-only 提案** で V/F pair 同定範式違反、§3.5 範式適用で F partner 全件追加同定して V/F 同期 patch に修正。1-shot ACCEPT 判定後の **patch 計画 review step が範式追加**。

### §3.4 新規 設計範式: cascade pair shift 順方向 既達主指標退行時の ACCEPT 判定範式

**概要**: η-1 §3.2 cascade pair hypothesis を **既達主指標退行検出時の判定範式** として **既達退行を許容するための判定基準** を確立する。η-9 §3.2 範式 (Y > X で revert) と組み合わせて使用、Y < X かつ **既達退行が cascade pair shift 順方向 (= 主指標解消の直接副作用)** なら **ACCEPT 範囲**。

**位置付け**: feedback_falsification_as_progress 範式の **既達主指標退行への適用**。「既達主指標 13 種完全維持」期待値が破られても、退行が cascade pair shift 順方向 (= 主指標解消 patch の直接副作用、新規 root cause ではない) なら deliverable と判定 → 後続 sub-bundle で復活処理。

**Detection 手順** (4 step):
1. **退行の Y count**: 既達主指標退行件数 = Y
2. **主指標解消の X count**: 主指標解消件数 = X
3. **退行 program の出自確認**: 退行発生 program が **本 sub-bundle で patch した program と一致するか** literal grep 確認
4. **退行 root cause の独立性確認**: 退行 metric の error 内容が **patch そのものの bug ではなく cascade pair shift** であることを log context で確認

**判定 matrix**:

| Y vs X | 退行出自 = patch program | 退行出自 ≠ patch program | 判定 |
|---|---|---|---|
| Y > X | (any) | (any) | **REVERT** (η-9 §3.2 範式) |
| Y < X | ✓ patch program | (n/a) | **ACCEPT** (cascade pair shift 順方向、本範式) |
| Y < X | n/a | ✓ 別 program | **要 review** (root cause 不明、AYA 判断仰ぐ) |

**η-11 適用例**:
- 退行 Y=5 (non-opaque uniforms outside a block)
- 主指標 X=11 (overlapping use of location 20)
- Y < X = 5 < 11 ✓
- 退行発生 program = Deferred Terrain Shader + SMAA Blending Weights ×4 = **本 sub-bundle patch program と完全一致** (terrainF + SMAABlendWeightsV) ✓
- 退行 error 内容 = `'non-opaque uniforms outside a block' : not allowed when using GLSL for Vulkan` = bare uniform 露出 (overlapping 解消で parse 次フェーズ進めるようになり、潜在の bare uniform error が露出) = cascade pair shift 順方向 ✓
- → **ACCEPT 範囲、AYA 判断「A で」(2026-06-02) で B?-η-12 scope に移管確定**

**handoff doc 記録価値**: 後続 sub-bundle で既達主指標退行を検出した際、本範式で判定基準を統一 = revert/ACCEPT の判断を矩形化 + AYA 判断仰ぐ範囲を最小化 (Y > X または 退行出自 ≠ patch program のみ AYA 判断必須)。

### §3.5 新規 設計範式: Agent 報告検証 step による V/F pair 同定範式違反検出範式

**概要**: η-10 §10.2 step 6 で確立された「Agent 報告検証 step (patch 化前 literal grep 検証)」を **V/F pair 同定範式違反検出の具体的適用範式** として確立する。Agent (Explore subagent) が V/F pair の V 側のみ patch 提案した場合、grep verify で F partner を追加同定して V/F 同期 patch に修正する範式。

**位置付け**: η-9 §3.1 V/F pair canonical partner 同定範式の **patch 計画 review step**。Agent は LLM ベースで V/F pair の対称性を見落とす場合あり (location=N の `out` 宣言を変更しても `in` 宣言を見落とすと Layout match error 退行誘発)。Agent 報告検証 step で V/F pair 全件確認が範式化必須。

**Detection 手順** (3 step):
1. **Agent 報告抽出**: Agent (Explore subagent) の patch 提案 file list 抽出
2. **V/F pair grep verify**: 提案 file が V 側 (`out <type> <var>`) または F 側 (`in <type> <var>`) のどちらかを literal grep で確認
3. **F (or V) partner 追加同定**: 各 V 側変更に対し `grep -rn "in <type> <var>" shaders/` で全 F partner 列挙 + 各 F 側変更に対し対応 V partner 列挙 (各変数名は通常 V/F pair で 1 対 1〜1 対 N)

**η-11 適用例**:
- Agent 提案: 7 V file (waterV/terrainV/skyV/cloudsV/sunDiscV/SMAABlendWeightsV/screenSpaceReflPostV) のみ patch、location=20→26
- V/F pair grep verify:
  - `grep "in vec4 refCoord"` → waterF + underWaterF (2 F file) 追加同定
  - `grep "in vec3 pos"` → terrainF 追加同定
  - `grep "in vec3 vary_HazeColor"` → skyF 追加同定
  - `grep "in vec3 vary_CloudColorSun"` → cloudsF 追加同定
  - `grep "in float sun_fade"` → sunDiscF 追加同定
  - `grep "in vec2 vary_pixcoord"` → SMAABlendWeightsF 追加同定
  - `grep "in vec3 camera_ray"` → screenSpaceReflPostF 追加同定
- F partner 全件追加同定: 7 V + 1 (underWaterF for refCoord 第 2 partner) + 7 (各 V の primary F partner) = 7 V + 8 F = 15 file
- → Agent 提案 7 file から 15 file に **patch 計画修正**
- Agent 提案のまま patch 実行していたら **Layout location qualifier must match 退行 +15 件** (V loc=26 vs F loc=20 mismatch) 誘発確定 → revert 必要だった

**charter §3 #1 担保**: V/F pair 同期 patch で V/F 両 side が同 location 値で揃う = Layout match error 防止 + GL `#else` path 全 file 不変。

**handoff doc 記録価値**: 後続 sub-bundle で Agent 報告利用時、本範式で V/F pair 同定範式違反を patch 化前に検出 = Layout match error 退行リスク予防、η-9 §3.1 範式の自然延長 + η-10 §3.3 1-shot ACCEPT 判定範式の patch 計画 review step として組み込み。

### §3.6 設計範式継承表 (η-1〜η-10 全件継承)

| η-N | 範式 | η-11 適用状況 |
|---|---|---|
| η-1 §3.1 | FrameViewProj guard wrap (set=0/binding=0) | η-11 適用外 (location qualifier 系) |
| η-1 §3.2 | cascade pair hypothesis (主指標 -X ↔ cascade +Y) | η-11 §3.4 で既達退行への適用範式確立、Y=5 < X=11 で ACCEPT |
| η-1 §3.3 | Agent 並列 disjoint scope | η-11 で Agent (Explore) 1 件投入 (11 program V/F pair + helper trace) |
| η-2 (a) §3.2 | UBO body member 順序 std140 完全一致 | η-11 適用外 (location qualifier 変更で UBO body 不触) |
| η-3 §3.1/§3.2 | Link failed root cause UBO body 差異解消 | η-11 適用外 (Link failed 0 維持) |
| η-4 §3.1/§3.2/§3.4 | nameless block × padding / cascade pair / UBO set=3 帯 | η-11 適用外 (location qualifier 系) |
| η-5 §3.1/§3.2/§3.3/§3.4 | multi-root-cause 同時対処 / 5th-level scope refinement / Agent depth trace / WEIGHT_LOCATION_DEFINED guard | η-11 適用外 (Phase 単一 root cause = overlapping のみ) |
| η-6 §3.1/§3.2/§3.3/§3.4/§3.5 | multi-cluster pilot / Agent 誤判定 → re-trace / nameless block × member / Agent capability limit 移管 / cascade pair 逆方向 | η-11 §3.5 で Agent 誤判定 (V-only) → re-trace + V/F pair partner 追加同定として継承拡張 |
| η-7 §3.1/§3.2/§3.3/§3.4/§3.5 | bvec2→uvec2 uint promote / program name collision / 5th-level cascade emergence / nameless block × member 第 4 例 / cascade pair 順方向 | η-7 §3.5 cascade pair 順方向は η-11 §3.4 既達退行判定範式の前駆 |
| η-8 §3.1/§3.2/§3.3/§3.4 | cinematic_bd override path 発見 / runtime preprocessed dump / mIndexedTextureChannels Vulkan-aware emit / UBO wrap 範式 reflectionProbeF 適用 | η-11 では runtime dump 未投入 (static trace で V/F + helper 同定可能、auto-attach 経路 llshadermgr.cpp grep で十分) |
| η-9 §3.1 | V/F pair canonical partner 同定範式 | η-11 §3.1 で **直接適用第 3 例** (auto-attach helper 経路まで canonical 判定範囲拡大) |
| η-9 §3.2 | Phase 1 falsification + 完全 revert + Phase 2 軌道修正 | η-11 では Phase 1 即 ACCEPT (1-shot 完結)、§3.4 で既達退行への判定範式拡張 |
| η-9 §3.3 | feedback_falsification_as_progress の sub-bundle 内 phase falsification 適用 | η-11 §3.4 で既達退行 ACCEPT 判定 + 後続 sub-bundle 移管として feedback_falsification_as_progress 範式適用 |
| η-9 §3.4 | feedback_admit_unknown の Phase 1 → Phase 2 切替 | η-11 適用外 (1-shot 完結) |
| η-10 §3.1 | handoff doc canonical 記載 着手前再検証範式 | η-11 §3.2 で **直接適用第 2 例** (canonical 判定範囲拡大 = attach 経路まで) |
| η-10 §3.2 | V/F pair canonical partner 同定範式 第 2 例 | η-11 §3.1 で第 3 例 (η-10 §3.2 が η-9 §3.1 の例 enumeration 範式継承) |
| η-10 §3.3 | 1-shot ACCEPT vs 2-phase 構成判定範式 | η-11 §3.3 で **直接適用第 2 例**、§3.5 で patch 計画 review step 拡張 |

**feedback memory 10 件全件適用確認**:
- feedback_doubt_self_first: η-11 §3.2 適用 (handoff doc canonical 記載 + Agent 報告に対する着手前再検証 + patch 化前 grep verify)
- feedback_root_cause_not_dump: 7 V + 8 F = 15 file V/F 同期 patch で V 規約 + F 規約 両 side 整合 (workaround 不使用、η-9 §3.3 falsification 履歴遵守 = canonical atmosphericsVarsV 触らず)
- feedback_shader_only_fast_iterate: shader cp + cache clear のみ (autobuild 不要、C++ touch 0)
- feedback_no_auto_commit: AYA 「commit して」明示指示後に commit
- feedback_no_claude_coauthor: commit message に Co-Authored-By 不在
- feedback_one_step_at_a_time: 着手前 trace → Agent 投入 → 報告検証 → patch → deploy → verify → AYA 判断仰ぐ → handoff doc 起草 を順次実施
- feedback_proactive_handoff: 本 handoff doc 起草
- feedback_no_scope_shrink: η-11 scope (overlapping 11 件のみ) は AYA 明示指示 = scope shrink でなく AYA 確定 scope、退行 non-opaque も AYA 「A で」判断で移管確定 = AYA 判断尊重
- feedback_admit_unknown: η-11 では仮説 2 連続外れず (Agent 報告 1 件 V-only 提案 → 即 grep verify で F partner 追加 = §3.5 範式で予防)、適用機会なし
- feedback_falsification_as_progress: η-11 §3.4 で既達退行 ACCEPT 判定 + 後続 sub-bundle 移管として適用

---

## §4 cold cache launch verify 詳細

### §4.1 verify cycle 構成 (1 cycle = 1-shot ACCEPT)

**Cycle 1 (Phase 1 verify = ACCEPT)**:
- 操作: 15 file patch → shader-only fast-iterate cp 15 file → shader_cache clear (0 件 verify) → AYA cold cache launch → 軽操作 → shutdown
- 結果:
  - `overlapping use of location 20` 11 → **0** (-11 ✓ 100% 完全達成)
  - `non-opaque uniforms outside a block` 0 → 5 ⚠️ (cascade pair shift 順方向、§3.4 範式適用で ACCEPT)
  - 12 種既達主指標 0 完全維持
  - 第10層 emergence 残系統 (SPIR-V 41 / normalMap 69 / depthMap 9) 完全 ±0 (η-12 移管継続)
  - clean shutdown (Goodbye 1 + Vulkan device destroyed 1 + FATAL/SIGSEGV/Aborted 0)
- 判定: **η-11 deliverable 確定 (1-shot ACCEPT)**、AYA 「A で」判断で既達退行を η-12 scope に移管

### §4.2 主指標 metric integrity self-check

- `overlapping use of location 20` -11 = 7 V file + 8 F file の location=20 → 26 整合直接効果 (11 program 全件、各 program の vertex stage で atmosphericsVarsV.glsl:28 vary_AdditiveColor (loc=20) と main shader file の out (loc=20) 衝突解消)
- `'location'` literal grep 52→45 = Layout match 0 + overlapping -11 + SPIR-V 41 同居で literal 数減 (cascade pair shift 内訳 = overlapping 解消の cascade pair shift 順方向)
- shader_cache 308→310 (+2) = link 成立 program 集合シフト (η-7 305 baseline + 5)
- parse failed 132→130 (-2) = overlapping 解消の cascade pair shift 順方向 (11 program 中 5 件は non-opaque 露出で parse 失敗継続、6 件は SPIR-V missing 等別系統で parse 失敗継続だが内訳シフト)

### §4.3 既達退行 (non-opaque uniforms outside a block) 5 件 root cause 同定

| # | program | stage | error LINE | 推定 root cause |
|---|---|---|---|---|
| 1 | Deferred Terrain Shader | fragment 0x8b31 | 0:1146 | terrainF.glsl 内 bare uniform (前 sub-bundle 未対処) |
| 2 | SMAA Blending Weights (Low) | vertex 0x8b30 | 0:1712 | SMAABlendWeightsV.glsl or SMAA.glsl 内 bare uniform |
| 3 | SMAA Blending Weights (Medium) | vertex 0x8b30 | 0:1712 | 同上 |
| 4 | SMAA Blending Weights (High) | vertex 0x8b30 | 0:1712 | 同上 |
| 5 | SMAA Blending Weights (Ultra) | vertex 0x8b30 | 0:1712 | 同上 |

全 5 件が η-11 で patch した program と完全一致 = §3.4 範式 cascade pair shift 順方向 確定。SMAA 4 件は同 line 1712 で同 root cause = 単一 bare uniform 露出 (SMAA.glsl helper の方が可能性高、SMAABlendWeightsV.glsl は location 整合のみで bare uniform 無し)。

### §4.4 shutdown clean verify

- Cycle 1 で:
  - `Goodbye!` 1 件 + `Vulkan device destroyed` 1 件
  - FATAL/SIGSEGV/Aborted 0/0/0

---

## §5 self-verify 19 項目 all green

| # | 項目 | 状態 |
|---|---|---|
| 1 | charter §3 #1 GL path byte-for-byte 不変担保 (15 file GL `#else` path 元宣言不変) | ✓ |
| 2 | shader file のみ touch (C++ touch 0) | ✓ |
| 3 | 主指標 `overlapping use of location 20` -11 literal grep verify | ✓ |
| 4 | 既達主指標 12 種 literal grep verify 全 0 維持 (non-opaque 1 件退行 AYA 「A で」判断で η-12 移管) | ✓ |
| 5 | parse failed integrity (132→130 -2 cascade pair shift 順方向) | ✓ |
| 6 | FATAL/SIGSEGV/Aborted 0 維持 | ✓ |
| 7 | clean shutdown (Goodbye 1 + Vulkan destroy 1) | ✓ |
| 8 | shader_cache baseline 確立 (η-10 308 → η-11 310、η-12 観測点) | ✓ |
| 9 | V/F pair canonical partner 同定範式 §3.1 直接適用第 3 例 (auto-attach helper 経路まで canonical 判定範囲拡大) | ✓ |
| 10 | handoff doc canonical 記載 着手前再検証範式 §3.2 直接適用第 2 例 | ✓ |
| 11 | 1-shot ACCEPT vs 2-phase 構成判定範式 §3.3 直接適用第 2 例 | ✓ |
| 12 | cascade pair shift 順方向 既達退行 ACCEPT 判定範式 §3.4 新規確立 (Y=5 < X=11 + 退行出自 = patch program 確認) | ✓ |
| 13 | Agent 報告検証 step V/F pair 同定範式違反検出範式 §3.5 新規確立 (Agent V-only 提案 → F partner 8 file 追加同定) | ✓ |
| 14 | η-9 §3.3 falsification 履歴遵守 (canonical atmosphericsVarsV.glsl:28 触らず、location=20 既使用 audit で safe location=26 選定) | ✓ |
| 15 | 過去 sub-bundle 既処理 file byte-for-byte 維持 (η-1〜η-10 全 file 不触、η-11 は 15 file 全件初 touch) | ✓ |
| 16 | skip list admission 範式継承 (15 file 全件 η-1〜η-10 全範囲外 初 touch、skip list 触れず) | ✓ |
| 17 | feedback_no_claude_coauthor 遵守 (commit message Co-Authored-By 不在) | ✓ |
| 18 | feedback_no_auto_commit 遵守 (AYA 「commit して」明示指示後に commit) | ✓ |
| 19 | handoff doc 別 commit (η-10-complete `9246142639` 範式継承) | ✓ |

---

## §6 設計範式継承表

(§3.6 と内容重複のため §3.6 参照、feedback memory 10 件全件適用確認 + prior sub-bundle 30+ 件継承 + 新規 2 件 §3.4/§3.5)

---

## §7 risks

| # | risk | mitigation |
|---|---|---|
| 1 | cascade pair shift 順方向 既達退行 ACCEPT 判定範式 §3.4 の **濫用** (Y > X でも退行出自 = patch program で誤適用) | §3.4 Detection 手順 4 step 全件確認 + Y > X は **必ず REVERT** (η-9 §3.2 範式) で固定、判定 matrix 厳守 |
| 2 | Agent 報告検証 step V/F pair 同定範式違反検出範式 §3.5 の **適用範囲過大** (全 Agent 報告で V/F pair grep verify 必須化は工数膨張) | §3.5 適用は location qualifier 系 (in/out 宣言) のみ、uniform/sampler 等の system 系は対象外 |
| 3 | η-9 §3.1 V/F pair canonical partner 同定範式 第 3 例の **auto-attach helper 経路まで canonical 判定範囲拡大** で attach 経路 trace 工数増 | η-12+ で attach 経路 grep を着手前 trace 標準化 (llshadermgr.cpp `attachVertexObject` + llviewershadermgr.cpp `mShaderFiles.push_back` 両方の literal grep) |
| 4 | non-opaque 既達退行 5 件 (Deferred Terrain F + SMAA Blending Weights V ×4) **η-12 移管** で残量増 | η-12 着手前 trace で non-opaque 復活を最優先 + SPIR-V 41 等と混在 = sub-bundle 細分化方針継続 (1 系統 = 1 sub-bundle) |
| 5 | location=26 を **safe location** として確立した範式 (15 file 全件 26 統一) で **後続 sub-bundle で 26 を他用途で割当不可** = location 番号余り減少 | η-12+ で safe location 番号選定時に **26 既使用** を着手前 trace で明示確認、η-9 §3.3 falsification 履歴 (location=20 既使用) と同様の audit |
| 6 | SMAA 4 件同 line 1712 で同 root cause 推定 = **単一 bare uniform 露出** だが SMAA.glsl helper か SMAABlendWeightsV.glsl 内か **特定未確定** | η-12 着手前 trace で SMAA.glsl + SMAABlendWeightsV.glsl 両方 grep + line 1712 周辺 helper concat 構造解析 |
| 7 | `'location'` literal grep 52→45 -7 = Layout 0 + overlapping -11 + SPIR-V 41 で internal consistency 検証 (45 = 41 + 0 + 4 残) で **残 4 件の location literal が何の error か未特定** | η-12 着手前 trace で `grep -B 2 "'location'"` で残 4 件 context 抽出、新 metric 系統発見可能性 |
| 8 | charter §3 #1 担保が **既存 numerical literal 変更** (location 番号 20→26、15 file) で揺らぐ懸念 = **insertions-only 範式の拡張延長** として GL `#else` 完全不変なら担保継続 | η-9/η-10 §3.2 範式で確立済 = GL `#else` path は数値含め完全不変、Vulkan path 内 layout(location=N) 数値のみ変更 = 既範式の自然延長と整理、η-11 で 15 file 一括適用で範式実証 |
| 9 | shader file のみ touch 範式の **runtime emit (C++) 系 root cause 取りこぼし** = η-8 §3.3 で 1 例発生、η-9/η-10/η-11 で 0 だが η-12 SPIR-V 41 件対処時に再発可能性高 | η-12 着手前に **C++ shader 関連 file (llshadermgr.cpp / llglslshader.cpp / llviewershadermgr.cpp) の bare uniform / sampler emit 系列挙** を grep で先回り (η-10 §7 #8 継承) |
| 10 | cinematic_bd directory 全 .glsl audit (§10.4 = η-8 から継承、η-9・η-10・η-11 未実施) **未実施** | η-12 着手前 or 完遂後の別 phase として強く推奨 (η-8 §3.1 cinematic_bd override path 発見範式の予防的展開、η-10 §7 #9 継承) |
| 11 | shader_cache 件数 (η-7 305 baseline → η-11 310) **η-12 観測点切替** | η-12 で 310+ 維持 + 第12層 emergence 観測 |
| 12 | Phase 単一 deliverable が **15 file 1 機能** で η-N sub-bundle scope として中規模 = 「sub-bundle 細分化」と「scope サイズ」のバランス確認必要 | feedback_no_scope_shrink 違反でなく **AYA 明示指示 (A 系統 = overlapping 11 件のみで進めて)** = scope shrink でなく AYA 確定 scope、15 file は V/F pair 同期で **必要十分** の patch 範囲、scope 拡大ではない (η-10 §7 #11 継承) |
| 13 | feedback_no_scope_shrink 違反懸念 (handoff §10.1 4 候補中 1 系統のみ scope 化 + non-opaque 退行を η-12 移管) | AYA 「A で」明示指示下 scope 確定 + 「A で」判断で non-opaque 移管確定 (2026-06-02)、残 3 候補 + non-opaque 5 件は η-12+ 移管 = scope shrink でなく **scope 分割 + 副次効果移管** (η-10 §7 #12 継承) |
| 14 | cascade exposure 第10層 残 (SPIR-V 41 + normalMap 69 + depthMap 9 = 計 119 件 + 新規 non-opaque 5 件 = 124 件) η-12 移管 = **sub-bundle 残量大** | η-12 着手前 trace で系統別 Phase 分割継続 (non-opaque 復活 = 1 Phase / SPIR-V = 1 Phase / normalMap+depthMap = 1 Phase)、η-12 着手 scope は AYA 判断 |
| 15 | location=20 → 26 で **15 file 一気の数値変更** は patch 規模が η-9 (1 file +5/-3) / η-10 (1 file +3/-3) より大幅増 = review 工数増 | git diff --stat + git diff content grep で全件 `layout(location=20)` → `layout(location=26)` のみ確認、numeric-only edit pattern で review 容易性確保 |

---

## §8 commit history

| commit | sub-bundle | scope |
|---|---|---|
| (η-11 patch 予定) | B?-η-11 patch | 7 V + 8 F = 15 file Vulkan path location 20 → 26 整合 (overlapping use of location 20 11→0 達成) |
| `9246142639` | B?-η-10 handoff doc | (η-10 完遂 handoff、η-11 着手境界引継) |
| `e1d5ffd98c` | B?-η-10 patch | diffuseAlphaMaskF.glsl Vulkan path location 4/2/0 → 0/1/2 整合 (1 file +3/-3) |
| `6fee4a818b` | B?-η-9 handoff doc | (η-9 完遂 handoff) |
| `50cb5f6713` | B?-η-9 patch | avatarEyesV.glsl Vulkan path location 4/2/0 → 0/1/2 整合 (1 file +5/-3) |
| `6994eba271` | B?-η-8 handoff doc | (η-8 完遂 handoff) |
| `2542905ce0` | B?-η-8 patch | non-opaque 1 + 'binding' 25 完全達成 (3 file +60/-1 = C++ 1 + shader 2) |
| `b1e8689634` | B?-η-7 handoff doc | (η-7 完遂 handoff) |
| `874d1a7252` | B?-η-7 patch | nameless / weight redefinition / non-opaque 67% (4 file) |

---

## §9 file inventory (η-11 patch 15 file 内訳)

| # | file | 変更内容 | 行数 | 範式適用 |
|---|---|---|---|---|
| 1 | `indra/newview/app_settings/shaders/class1/environment/waterV.glsl` | Vulkan path location 20 → 26 (refCoord) | +1/-1 | §3.1 V/F pair canonical partner 同定範式 第 3 例 (atmosphericsVarsV を canonical として尊重、refCoord 移動) |
| 2 | `indra/newview/app_settings/shaders/class3/environment/waterF.glsl` | Vulkan path location 20 → 26 (refCoord) | +1/-1 | §3.5 V/F pair 同期 patch (waterV partner) |
| 3 | `indra/newview/app_settings/shaders/class3/environment/underWaterF.glsl` | Vulkan path location 20 → 26 (refCoord) | +1/-1 | §3.5 V/F pair 同期 patch (waterV 第 2 partner、Underwater Shader 専用) |
| 4 | `indra/newview/app_settings/shaders/class1/deferred/terrainV.glsl` | Vulkan path location 20 → 26 (pos) | +1/-1 | §3.1 V/F pair canonical partner 同定範式 第 3 例 |
| 5 | `indra/newview/app_settings/shaders/class1/deferred/terrainF.glsl` | Vulkan path location 20 → 26 (pos) | +1/-1 | §3.5 V/F pair 同期 patch (terrainV partner) |
| 6 | `indra/newview/app_settings/shaders/class1/deferred/skyV.glsl` | Vulkan path location 20 → 26 (vary_HazeColor) | +1/-1 | §3.1 V/F pair canonical partner 同定範式 第 3 例 (2 program 共有 = EnvMap + WLSky) |
| 7 | `indra/newview/app_settings/shaders/class1/deferred/skyF.glsl` | Vulkan path location 20 → 26 (vary_HazeColor) | +1/-1 | §3.5 V/F pair 同期 patch (skyV partner) |
| 8 | `indra/newview/app_settings/shaders/class1/deferred/cloudsV.glsl` | Vulkan path location 20 → 26 (vary_CloudColorSun) | +1/-1 | §3.1 V/F pair canonical partner 同定範式 第 3 例 |
| 9 | `indra/newview/app_settings/shaders/class1/deferred/cloudsF.glsl` | Vulkan path location 20 → 26 (vary_CloudColorSun) | +1/-1 | §3.5 V/F pair 同期 patch (cloudsV partner) |
| 10 | `indra/newview/app_settings/shaders/class1/deferred/sunDiscV.glsl` | Vulkan path location 20 → 26 (sun_fade) | +1/-1 | §3.1 V/F pair canonical partner 同定範式 第 3 例 |
| 11 | `indra/newview/app_settings/shaders/class1/deferred/sunDiscF.glsl` | Vulkan path location 20 → 26 (sun_fade) | +1/-1 | §3.5 V/F pair 同期 patch (sunDiscV partner) |
| 12 | `indra/newview/app_settings/shaders/class1/deferred/SMAABlendWeightsV.glsl` | Vulkan path location 20 → 26 (vary_pixcoord) | +1/-1 | §3.1 V/F pair canonical partner 同定範式 第 3 例 (4 program 共有 = SMAA Blending Weights Low/Medium/High/Ultra) |
| 13 | `indra/newview/app_settings/shaders/class1/deferred/SMAABlendWeightsF.glsl` | Vulkan path location 20 → 26 (vary_pixcoord) | +1/-1 | §3.5 V/F pair 同期 patch (SMAABlendWeightsV partner) |
| 14 | `indra/newview/app_settings/shaders/class3/deferred/screenSpaceReflPostV.glsl` | Vulkan path location 20 → 26 (camera_ray) | +1/-1 | §3.1 V/F pair canonical partner 同定範式 第 3 例 |
| 15 | `indra/newview/app_settings/shaders/class3/deferred/screenSpaceReflPostF.glsl` | Vulkan path location 20 → 26 (camera_ray) | +1/-1 | §3.5 V/F pair 同期 patch (screenSpaceReflPostV partner) |

**計**: 15 file +15/-15、shader file のみ、C++ touch 0

**overlap file 確認**:
- 全 15 file: η-1〜η-10 全範囲外 (η-11 で全件初 touch、skip list 触れず)
- atmosphericsVarsV.glsl / atmosphericsVarsF.glsl: η-11 で参照のみ (canonical 同定)、touch 0
- llshadermgr.cpp / llviewershadermgr.cpp: η-11 で参照のみ (auto-attach 経路同定 + program file 列挙)、touch 0
- SMAA.glsl: η-11 で参照のみ (layout 無し確認)、touch 0

---

## §10 次 sub-bundle B?-η-12 推奨 scope

### §10.1 確定 scope: 既達退行 non-opaque 復活 + 第10層 emergence 残全件

| metric | η-11 末件数 | 推定 root cause | 推奨対処 |
|---|---|---|---|
| **non-opaque uniforms outside a block** | **5** | terrainF.glsl + SMAA.glsl or SMAABlendWeightsV.glsl 内 bare uniform 露出 (η-11 cascade pair shift 順方向) | 専用 phase (bare uniform 探索 + UBO wrap or 直接 layout(set/binding) 追加、η-8 §3.4 UBO wrap 範式継承) |
| SPIR-V requires location | 41 | in/out 宣言の location 番号未指定 (mass) | C++ runtime emit Vulkan-aware 化 (η-8 §3.3 範式) or 一括 wrap script |
| 'normalMap' redefinition | 69 | (η-1〜η-11 未対処、新 root cause 系統) | 専用 phase (Agent 並列 disjoint scope η-1 §3.3) |
| 'depthMap' redefinition | 9 | 同上 | 同上 |
| missing #endif / parse failed | cascade 縮小傾向 | cascade pair shift | 主指標解消後の自然減 |

### §10.2 B?-η-12 着手前 trace 範式 8 ステップ (η-11 §3.4/§3.5 範式追加)

1. **literal grep**: `grep -c "non-opaque uniforms outside a block" log` + `grep -c "SPIR-V requires" log` + `grep -c "'normalMap'" log` + `grep -c "'depthMap'" log` + `grep -c "overlapping use of location 20" log` (η-11 達成維持 = 0 確認) + `grep -c "Layout location qualifier" log` (η-10 達成維持 = 0 確認)
2. **log context 抽出**: 各 program の error LINE × 件数 + stage type 番号 (0x8b30 vertex / 0x8b31 fragment) を **必ず併記** (η-6 §3.2 範式)
3. **V/F pair 同定** (η-9 §3.1 / η-10 §3.2 / **η-11 §3.1 第 3 例 拡張**): location 系は **`llviewershadermgr.cpp` で `mFragmentFiles.push_back` + `mVertexFiles.push_back` 同 program 内列挙** で V/F file 確定 + **他 program での共有確認** で canonical partner 同定 + **auto-attach helper 経路 (llshadermgr.cpp `attachVertexObject` / `attachFragmentObject`) も grep で同定** (η-11 §3.1 拡張)
4. **handoff doc canonical 記載再検証** (η-10 §3.1 / **η-11 §3.2 範式**): 前 sub-bundle (η-11) の canonical 同定記録を **着手時 grep で再確認** = canonical 判定が context 依存であることを前提、location=26 既使用 audit 必須
5. **Agent 並列 disjoint scope** (η-1 §3.3 範式): metric 系統別に **root cause 軸** で分割 (non-opaque / SPIR-V missing / normalMap / depthMap 各別 Agent)
6. **Agent 報告検証 step** (η-6 §3.2 / **η-11 §3.5 範式 V/F pair 同定範式違反検出**): stage type / source file 同定の literal grep 検証 + **location qualifier 系は V/F pair partner 全件確認** を patch 化前に必須実施
7. **1-shot ACCEPT vs 2-phase 構成判定** (η-10 §3.3 / **η-11 §3.3 第 2 例 拡張**): 着手前 trace で canonical 確定なら 1-shot、不確定なら 2-phase 構成で Phase 1 falsification 許容
8. **cascade pair shift 既達退行 ACCEPT 判定** (**η-11 §3.4 範式**): verify 後に既達退行検出時、Y < X + 退行出自 = patch program 確認で ACCEPT、Y > X または 退行出自 ≠ patch program は AYA 判断仰ぐ

### §10.3 B?-η-12 完遂後の想定 cascade exposure 第12層

- non-opaque 5 件解消想定 = parse failed -5 程度
- SPIR-V missing 41 件 → C++ runtime emit Vulkan-aware 化で大量解消想定
- normalMap / depthMap redefinition 78 件 → 別 sub-bundle 分割推奨 (B?-η-13 移管候補)
- shader_cache 件数: η-12 で 310+ 維持 + 第12層 emergence 観測

### §10.4 cinematic_bd 系統 systematic audit 推奨 (η-8 から継承、η-9・η-10・η-11 未実施)

η-8 §3.1 cinematic_bd override path 発見範式で shadowUtil のみ個別検出。cinematic_bd directory 内 .glsl file 全件で類似 override + 未 wrap bare uniform / 未整合 location 残存可能性。B?-η-12 着手前 or 完遂後の別 phase として:

1. `find indra/newview/app_settings/shaders/cinematic_bd -name "*.glsl"` で全件列挙
2. 各 file の bare uniform / sampler / `layout` 欠落 + location qualifier 不整合 grep audit
3. 該当 file を class1/class2/class3 版 patch と同 set/binding + location で wrap / 整合 (mutually exclusive 担保)

η-8 で発生した「想定外 cinematic_bd 起源 metric 露出」と η-11 で発生した cascade pair shift 順方向 (non-opaque 5 件) の **両方を予防的に解消**する。

### §10.5 runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画継承

η-9 / η-10 / η-11 では未投入 (static trace で V/F + auto-attach helper 同定可能だった)。η-12 で **SMAA 4 件同 line 1712 の bare uniform** root cause が SMAA.glsl helper concat 経由で static trace 不能な場合、η-8 §3.2 範式再投入 = `llglslshader.cpp` parse-fail 路径に LL_WARNS diag dump 一時追加 → sub-bundle 完遂時に **必ず除去** (η-8 baseline 完全復元)。

### §10.6 location=26 既使用 audit 範式 (η-11 §3.1 第 3 例で新規確立)

η-11 で 15 file 全件 location=26 統一 = 後続 sub-bundle で location=26 を **他用途で割当不可** に変化。B?-η-12+ で **safe location 番号選定時の必須 audit step**:

1. `grep -rn "layout(location=N)" indra/newview/app_settings/shaders/` で全件列挙
2. 各 N の使用 file + program 同定
3. **完全未使用 N** または **本 sub-bundle 対象 program と重複しない N** を safe location 候補化
4. η-9 §3.3 falsification 履歴 (location=20 既使用) + η-11 §3.1 (location=26 既使用) を都度更新

---

## §11 観測点

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-11 310) | ✓ η-11 で baseline 更新 | B?-η-12 で 310+ 維持 + 第12層 emergence 観測 |
| 2 | V/F pair canonical partner 同定範式 第 3 例 (§3.1) 適用 | 適用済 (15 file V/F 同期、auto-attach helper 経路 canonical 同定) | B?-η-12 でも location qualifier 系で同範式継続適用 + attach 経路まで canonical 判定範囲拡大 |
| 3 | handoff doc canonical 記載 着手前再検証範式 (§3.2) 第 2 例適用 | 適用済 | B?-η-12 でも前 sub-bundle canonical 記載の再 grep 必須 |
| 4 | 1-shot ACCEPT vs 2-phase 構成判定範式 (§3.3) 第 2 例適用 | 適用済 (1-shot) | B?-η-12 で着手前 trace 確定度で判定 |
| 5 | cascade pair shift 順方向 既達退行 ACCEPT 判定範式 (§3.4) 新規確立 | 適用済 (Y=5 < X=11、退行出自 = patch program、ACCEPT) | B?-η-12 で既達退行検出時に Detection 手順 4 step 適用 |
| 6 | Agent 報告検証 step V/F pair 同定範式違反検出範式 (§3.5) 新規確立 | 適用済 (Agent V-only 提案 → F partner 8 file 追加同定) | B?-η-12 でも location qualifier 系で同範式継続適用 |
| 7 | set/binding allocation 連続割当 (η-8 末: 14-59 + 100-103) | η-11 で binding touch なし | B?-η-12 で SPIR-V missing 41 件対処時に binding 触る場合は 60+ 連続割当開始 |
| 8 | C++ touch 0 維持 (η-8 で 1 例発生、η-9/η-10/η-11 で 0) | η-11 達成 | B?-η-12 で SPIR-V missing 41 件対処時に C++ touch 復活可能性 (η-8 §3.3 範式) |
| 9 | 既達主指標完全維持 (η-11 で 13→12 種、non-opaque η-12 移管) | △ η-11 で 1 件退行 AYA 判断で移管 | B?-η-12 で non-opaque 復活 + 12 種維持 + 残系統 scope |
| 10 | cinematic_bd directory 全 .glsl audit (§10.4) | 未実施 (η-8 から継承、η-9/η-10/η-11 全て未実施) | B?-η-12 着手前 or 完遂後の別 phase として推奨 |
| 11 | runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画 | η-9/η-10/η-11 未投入 | B?-η-12 で SMAA 4 件 line 1712 bare uniform root cause が static trace 不能な場合に再投入 + sub-bundle 完遂時除去 |
| 12 | location=26 既使用 audit (§10.6 新規) | η-11 で 15 file 全件 location=26 統一 | B?-η-12+ で safe location 番号選定時に必須 audit step |
| 13 | cascade exposure 第10層 残 (SPIR-V 41 + normalMap 69 + depthMap 9 = 119 件) + 第11層 emergence (non-opaque 5 件) = 計 124 件 η-12 移管 | sub-bundle 残量大 | B?-η-12 着手前 trace で系統別 Phase 分割継続推奨 (non-opaque = 1 Phase 第 1 候補) |

---

**handoff doc 完。次 sub-bundle B?-η-12 着手は本 doc §10 推奨 scope を起点として、fresh context で実施。**
