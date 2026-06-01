# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-6 完遂 handoff

**status**: B?-η-6 完遂 → 次 sub-bundle B?-η-7 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: `e915af26fe` (2026-06-02)
**handoff doc commit**: 本 doc (B?-η-5-complete `d6afcfaee3` 範式継承)
**勤続範式継承**: B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-6 scope = 第7層 emergence `non-opaque uniforms outside a block` 60 件 主指標解消 + phase 2 自作 bug 3 件遡及 fix、**3 phase 構成**:

- **phase 1** (主指標 non-opaque 60 件対処): 24 file +217/-1、6 Cluster pilot + Agent 並列展開
  - Cluster A pilot: `SMAA.glsl` SMAAParamUBO_Legacy (set=3, binding=14)
  - Cluster B Agent: Glow 3 file (binding=18/19/20)
  - Cluster C pilot: `reflectionProbeF.glsl` ReflectionProbeUBO_Legacy (binding=17)
  - Cluster D Claude 再 trace: `pbrShadowAlphaMaskV` + `avatarAlphaShadowV` (binding=21/22) (Agent A 初期 trace 誤判定 → Claude re-patch)
  - Cluster E Agent: dofCombineF/exposureF/luminanceF/motionBlurF/screenSpaceReflPostF/skinSSSF 6 file (binding=24-30)
  - Cluster F Agent: clipF/normgenF/normaldebugV/snapshotFrameF/underWaterF/previewV/simpleColorF/starsF/sunDiscF/moonF/starsV 11 file (binding=32-45)
- **phase 2-B** (verify 後 自作 bug 1 件 遡及 fix): 1 file +18/-0
  - `pbrShadowAlphaMaskV.glsl` HAS_SKIN ELSE branch FrameViewProj guard wrap (η-5 (a) scope leak fix、η-1 §3.1 範式継承)
- **phase 2-A** (verify 後 自作 bug 2 件 遡及 fix): 2 file +52/-0
  - `underWaterF.glsl` UnderWaterFParamUBO_Legacy 内 `lightDir` `eyeVec` per-block 固有化 (waterV.glsl bare uniform と global scope collision)
  - `skinSSSF.glsl` SkinSSSPrototypeFParamUBO_Legacy 内 `aya_visual_realism_enabled` per-block 固有化 (atmosphericsFuncs.glsl AtmoExtraUBO_Legacy member との global scope collision)
- **phase 2-C** (verify 後 残 non-opaque 15 件中 13 件対処): 10 file +88/-1
  - exoVignetteF / pathfindingV / pathfindingNoNormalV / glowcombineF / occlusionCubeV / radianceGenF / irradianceGenF / pbropaqueV / avatarF / velocityV (set=3, binding=46-55)
  - LINE 0:570 Avatar Eyes / LINE 0:1969 reflectionmipF / LINE 0:1700 RLVa Sphere 3 件 η-7 移管

**計**: 34 file +348/-2 (shader file のみ、C++ touch 0)

**主指標達成**:
- `non-opaque uniforms outside a block` 60 → **3** ✓ -57 / 95% 解消 (主 scope 達成)
- `undeclared identifier` 0 → **0** ✓ phase 2-B FrameViewProj 追加で 0 維持
- `nameless block ... global scope` 0 → **1** (phase 2-A 2 件中 1 件解消、1 件残 = η-7 移管)
- `Cannot reuse block name within the same interface` 0 → **0** ✓ B?-η-5 達成完全維持

**既達主指標完全維持**: `'weight4' redefinition` 0 / `'size' undeclared` 0 / `Link failed` 0 / `GBufferInfo redefinition struct` 0 / `'#'` preprocessor 0

**第7層 emergence 新規露出**: `'weight'` redefinition +1 / `'binding'` +4 / `'location'` +8 (link 成立 program 集合シフトによる、B?-η-7 移管対象)

---

## §2 完遂結果 metric (vs B?-η-5 baseline)

| metric | η-5 baseline | η-6 phase 1 (推定) | η-6 phase 2 (commit) | Δ vs η-5 | 判定 |
|---|---|---|---|---|---|
| non-opaque uniforms outside a block | 60 | 15 | **3** | -57 | ✓ 95% 主 scope 達成 |
| undeclared identifier | 0 | 2 | **0** | ±0 | ✓ phase 2-B 自作 bug 遡及 fix |
| nameless block ... global scope | 0 | 2 | **1** | +1 | phase 2-A 部分達成、η-7 移管 1 件 |
| Cannot reuse block name | 0 | 0 | 0 | ±0 | ✓ η-5 達成維持 |
| 'weight4' redefinition | 0 | 0 | 0 | ±0 | ✓ η-5 達成維持 |
| 'weight' redefinition | 0 | 0 | 1 | +1 | 第7層 emergence 新規露出 (η-7 移管) |
| 'size' undeclared | 0 | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| Link failed | 0 | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| '#' preprocessor | 0 | 0 | 0 | ±0 | ✓ ε 達成維持 |
| missing #endif | 87 | (測定割愛) | 90 | +3 | cascade 内訳変化 |
| parse failed | 179 | (測定割愛) | 137 | -42 | cascade 内訳大幅縮小 |
| redefinition (all) | 79 | (測定割愛) | 81 | +2 | 'weight' +1 + nameless 副次 +1 |
| 'binding' | 21 | (測定割愛) | 25 | +4 | 第7層 emergence 局所 |
| 'location' | 19 | (測定割愛) | 27 | +8 | 第7層 emergence 局所 |
| shader_cache 件数 | 263 | (測定割愛) | **302** | +39 | link 成立 program 集合大幅シフト |
| FATAL/SIGSEGV/Aborted | 0 | 0 | 0 | ±0 | ✓ |
| Goodbye / Vulkan device destroyed / instance destroyed / status: stopped | 1/1/1/1 | 1/1/1/1 | 1/1/1/1 | ±0 | ✓ clean shutdown |

---

## §3 設計範式

### §3.1 新規 設計範式: multi-cluster pilot + Agent 並列展開範式 (file-cluster 軸 disjoint scope)

**概要**: handoff §10 確定 root cause が **同一種類** (本例: non-opaque uniforms) で **多数 file** に散在する場合、root cause 種類でなく **file cluster** で 6 Cluster (A-F) に分割、pilot file (Claude 自力で patch literal 確立) + Agent disjoint scope 並列展開で同時対処する範式。

**位置付け**: η-5 §3.1 multi-root-cause 同時対処範式 (root cause 軸) の **file-cluster 軸拡張**、η-1 §3.3 Agent 並列 disjoint scope 範式の cluster-level 進化。

**適用条件**:
- handoff §10 主指標が **単一種類** (= patch literal pattern が同形)
- 対象 file が **15+ file 程度** で 1 Agent では context overflow リスクあり
- file 間 cross-dependency 無し (set/binding allocation のみ前広に管理)

**η-6 適用例** (phase 1 で 24 file scope を 6 Cluster 分割):

| Cluster | 対象 file | 担当 | binding 帯 |
|---|---|---|---|
| A | SMAA.glsl pilot | Claude 自力 | 14 |
| B | Glow 3 file | Agent B | 18-20 |
| C | reflectionProbeF.glsl pilot | Claude 自力 | 17 |
| D | pbrShadowAlphaMaskV + avatarAlphaShadowV | Claude 再 trace | 21-22 |
| E | dofCombineF/exposureF/luminanceF/motionBlurF/screenSpaceReflPostF/skinSSSF 6 file | Agent E | 24-30 |
| F | clipF/normgenF/normaldebugV/snapshotFrameF/underWaterF/previewV/simpleColorF/starsF/sunDiscF/moonF/starsV 11 file | Agent F | 32-45 |

**set/binding allocation 規則** (本範式付随):
- set=3 (legacy per-program UBO 帯) で binding を Cluster 別に **連続割当** (14-22 / 24-30 / 32-45 / 46-55)
- Cluster 間に **空き帯** (16, 23, 31) を意図的に挟み、phase 2 / 後 sub-bundle 用 reservation
- binding 衝突 0 を Agent 投入前に table 起草で担保

**Trade-off**:
- 利点: 24 file scope を 4 Agent + 2 Claude pilot で 1 sub-bundle 完了 (B?-ζ/η-1/η-2 (a) 等の 1 Agent 全 file パターンに対し context 分散)
- 注意: Cluster 境界選定 (file 数 + dependency) を pre-trace で確定する必要、Cluster D 初期誤判定例 (§3.2) は本範式の弱点

### §3.2 新規 設計範式: Agent 初期 trace 誤判定検出 → log stage type literal grep 再 trace 範式

**概要**: Agent 投入で得た trace 結果が **stage type / source file 同定で誤判定** していた場合、AYAstorm log の stage type 番号 (0x8b30 = fragment / 0x8b31 = vertex) を literal grep で再確定、Claude 自力 re-patch に切替える範式。

**位置付け**: η-5 §3.3 Agent depth trace 投入閾値範式の **誤判定検出補正形**、feedback_admit_unknown 範式 (Agent 結論を「正解」と受入れず疑う) の sub-bundle 内構造化、feedback_doubt_self_first 範式 (Agent 結論も「自分の仮説」と同等に疑う) の applied form。

**Agent 誤判定検出フロー** (3 step):
1. **Agent 報告受領**: 報告 file の patch literal 案を仮置
2. **検証 trace**: 同定 file の patch literal 適用前に、報告 stage type と log の literal 一致確認 (`grep -B 1 "<error>" log | grep -oE 'stage [0-9a-fx]+'`)
3. **不一致時**: Agent 結論破棄、log の literal stage type から逆引きで正源 file 再特定、Claude 自力 re-trace

**η-6 適用例** (Cluster D):
- Agent A 初期 trace: `Deferred GLTF Shadow Alpha Mask Shader` LINE 341/417/491 を **`pbrShadowAlphaMaskF.glsl` fragment 起源** と報告
- 検証 grep: log の error 行に **stage 0x8b31** (vertex) 明示
- Agent 報告破棄: pbrShadowAlphaMaskF.glsl に `shadow_target_width` 宣言無し (fragment 側未参照)
- Claude 再 trace: stage 0x8b31 + program 「Shadow Alpha Mask」 → `pbrShadowAlphaMaskV.glsl` vertex 起源と確定
- Claude re-patch: `pbrShadowAlphaMaskV.glsl` + `avatarAlphaShadowV.glsl` 2 file UBO wrap

**Trade-off**:
- 利点: Agent 誤判定で η-4 主指標 (`nameless block`) 退行を回避 (pbrShadowAlphaMaskF に新 UBO 追加で AtmoExtraUBO_Legacy collision を引き起こす危険を未然防止)
- 注意: 本範式適用は Agent 投入直後の検証 step が必須、検証無しに Agent 報告を patch 化すると η-4 主指標退行リスク

### §3.3 新規 設計範式: η-4 §3.2 (nameless block member global scope export 衝突 detection) の data member 適用 (padding 系の対称形)

**概要**: η-4 §3.2 で確立した **nameless block padding member 固有化範式** を、η-5 §3.4 で `lastMatrixPalette` 等 通常 member へ拡張済 → η-6 で **bare uniform と nameless block member の global scope collision** に第三適用。

**位置付け**: η-4 §3.2 (padding member) → η-5 §3.4 (通常 data member) → η-6 §3.3 (**bare uniform と data member 衝突**) の **member-level 対称形拡張第 3 例**。

**新規 collision type** (η-6 phase 2-A):
- 過去 = nameless block A の member ↔ nameless block B の同名 member (η-4, η-5)
- **新規 = nameless block の member ↔ 他 file の bare uniform** (η-6)
- 理由: nameless interface block member は GLSL/Vulkan profile で **global scope に exposed**、bare uniform も同 global scope に存在 → 同名で collision

**Detection 手順** (η-4 §3.2 完全継承 + η-6 拡張):
- (a) `nameless block ... global scope` エラー件数 × 発生 program で attach 関係 trace
- (b) attach される全 file の nameless block member 名 literal grep + **bare uniform literal grep**
- (c) member 名 collision 検出 (nameless block × nameless block + **nameless block × bare uniform**)
- (d) per-block 固有化 rename (η-4 §3.2 範式同形)
- (e) GL path 不可触担保 (charter §3 #1)

**η-6 適用例** (phase 2-A):

| UBO | 衝突 member | collision 相手 | rename 後 |
|---|---|---|---|
| `UnderWaterFParamUBO_Legacy` | `lightDir` `eyeVec` | `waterV.glsl` bare `uniform vec3 lightDir / eyeVec` | `lightDir_underwater_legacy` / `eyeVec_underwater_legacy` |
| `SkinSSSPrototypeFParamUBO_Legacy` | `aya_visual_realism_enabled` | `atmosphericsFuncs.glsl` `AtmoExtraUBO_Legacy` member | `aya_visual_realism_enabled_skinsss_legacy` + `#define` alias |

**`#define` alias 範式**: skinSSSF.glsl では main() body が `aya_visual_realism_enabled` を参照しているため、Vulkan path 内に `#define aya_visual_realism_enabled aya_visual_realism_enabled_skinsss_legacy` を配置、main() body byte-for-byte 不変を担保。

### §3.4 新規 設計範式: Agent capability limit による η-7 移管判断範式 (runtime preprocessed dump 不可ケース)

**概要**: Agent 投入で trace 不可能な error (例: glslang preprocess 後の merged source line offset から逆引きで source file 同定不可) は **η-7 移管判断**、Agent capability limit を明示で next sub-bundle 着手前 trace 範式に組み込む。

**位置付け**: η-5 §3.3 Agent depth trace 投入閾値範式の **Agent capability limit 明示形**、feedback_admit_unknown 範式 (24 時間内に解決の ceiling) の sub-bundle boundary 適用。

**η-6 適用例** (phase 2-C):
- LINE 0:570 Avatar Eyes `Deferred Avatar Eyes` program で `non-opaque uniforms outside a block` 1 件
- Agent F trace: globalF / srgbF / atmosphericsVarsF / shadowUtil / gammaF / atmosphericsFuncs / atmosphericsF / waterFogF / diffuseF 9 helper file 全 inspect
- 全 helper の bare uniform は `#else` branch 内 (= phase 2 で UBO wrap 不要)
- Agent 結論: **runtime preprocessed source dump 不可** で LINE 0:570 の source file 同定不可
- 判断: η-7 で Agent depth trace 範式 (§3.3) 再投入 (preprocess option 等を Agent に指示)、本 sub-bundle scope outside

**移管判断条件**:
- Agent 報告 = capability limit 明示 (= source file 同定不可)
- 残 main metric への影響 = 1 件のみ (3 件中 1 件 = ~33% 残)、95% 主 scope 達成済
- 24 時間 ceiling 内に sub-bundle 完了優先

### §3.5 η-1 §3.2 cascade pair hypothesis 汎用形 1:1 比例検証

**η-6 検証**:
- 主指標 non-opaque -57 件 vs missing #endif +3 件 = cascade pair 1:1 比例「逆」 (主指標減少 + cascade pair 増加)
- 原因: phase 1 で 24 UBO wrap 追加 = `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 構造 24 セット追加 → preprocess fail 路径で `#endif` カウント不整合露出
- net 評価: parse failed -42 = (non-opaque -57) + (第7層 emergence net +15 = 'weight' +1 + binding +4 + location +8 + nameless +1 + redefinition +2 - 1) で整合
- 結論: cascade pair hypothesis は **patch 構造増加時に逆方向シフト** あり、main metric 主導の整合性は維持

---

## §4 cold cache launch verify metric 詳細 (vs B?-η-5 baseline)

### §4.1 主指標 (handoff §10 1 件 主 scope + 自作 bug 3 件)

| 主指標 | η-5 | η-6 commit | Δ | 内訳 |
|---|---|---|---|---|
| non-opaque uniforms outside a block | 60 | **3** | -57 ✓ | phase 1 6 Cluster 24 file UBO wrap (-45) + phase 2-C 10 file UBO wrap (-12)、残 3 件 = Avatar Eyes 0:570 (1) + RLVa Sphere 1700 (1) + reflectionmipF 1969 (1) = η-7 移管 |
| undeclared identifier | 0 | **0** | ±0 ✓ | phase 2-B 自作 bug 1 件 (η-5 (a) scope leak) で +2 退行 → phase 2-B pbrShadowAlphaMaskV HAS_SKIN ELSE branch FrameViewProj 追加で 0 維持 |
| nameless block ... global scope | 0 | **1** | +1 | phase 2-A 自作 bug 2 件 (UnderWaterFParamUBO_Legacy + SkinSSSPrototypeFParamUBO_Legacy) で +2 退行 → phase 2-A 2 file rename で 1 件解消、1 件残 (η-7 移管) |
| Cannot reuse block name | 0 | **0** | ±0 ✓ | η-5 達成完全維持 |

### §4.2 既達主指標完全維持

| 主指標 | 値 | 由来 sub-bundle |
|---|---|---|
| 'weight4' redefinition | 0 | B?-η-5 達成完全維持 |
| 'size' undeclared | 0 | B?-η-3 達成完全維持 |
| Link failed | 0 | B?-η-3 達成完全維持 |
| GBufferInfo redefinition struct | 0 | B?-ζ 達成完全維持 |
| '#' preprocessor | 0 | B?-ε 達成完全維持 |

### §4.3 第7層 emergence 新規露出分析

| metric | η-5 | η-6 | Δ | 評価 |
|---|---|---|---|---|
| 'weight' redefinition | 0 | 1 | +1 | 第7層 emergence 新規露出 (η-7 移管) |
| 'binding' | 21 | 25 | +4 | 第7層 emergence 局所、phase 1 / 2-C で UBO 14 個追加に伴う SPIR-V binding 衝突または overlapping binding 新規露出 |
| 'location' | 19 | 27 | +8 | 第7層 emergence 局所、SPIR-V location missing + overlapping location |
| shader_cache 件数 | 263 | 302 | +39 | link 成立 program 集合大幅シフト |

### §4.4 主指標 metric integrity self-check (B3 §12 literal grep 範式継承)

- 主指標 4 種 95% 達成 (-57 main + 自作 bug ±0 + 部分 +1)
- cascade pair 1:1 比例「逆」 (main -57 vs missing #endif +3) = patch 構造増加由来
- parse failed -42 = main -57 + 第7層 emergence net +15 で整合
- shader_cache +39 = link 成立 program 集合大幅シフト = -21 観測点完全解消継続維持

### §4.5 shutdown clean verify

- FATAL/SIGSEGV/Aborted: 0/0/0 ✓
- Goodbye! : 1 ✓
- Vulkan device destroyed: 1 ✓
- Vulkan instance destroyed: 1 ✓
- status: stopped: 1 ✓
- launch 起動成立: 2026-06-01T18:36:56Z + clean shutdown 18:37:41Z 45 秒

---

## §5 self-verify (B3 §12 + B?-η-5 §5 範式継承)

| # | 項目 | 結果 |
|---|---|---|
| 1 | charter §3 #1 GL path byte-for-byte 不変 担保 | ✓ (34 file 全 outer `#ifdef LL_VULKAN_GLSL ... #else <GL path> ... #endif` literal 不変) |
| 2 | shader file のみ編集 (C++ touch 0) | ✓ `git diff --stat HEAD~1 -- '*.cpp' '*.h'` 0 |
| 3 | 主指標 4 種 literal grep verify | ✓ non-opaque 3 + undeclared 0 + nameless 1 + Cannot reuse 0 |
| 4 | 既達主指標 5 種 literal grep verify | ✓ weight4 0 + 'size' 0 + Link failed 0 + GBufferInfo 0 + '#' 0 全 ±0 |
| 5 | cascade pair 1:1 比例検証 (汎用形 逆方向シフト含) | ✓ missing #endif +3 vs main -57 = patch 構造増加由来 |
| 6 | parse failed integrity | ✓ -42 = main -57 + 第7層 emergence net +15 |
| 7 | shader_cache 件数 | ✓ 302 (+39 = link 成立 program 集合大幅シフト) |
| 8 | FATAL/SIGSEGV/Aborted 0 維持 | ✓ 0/0/0 |
| 9 | clean shutdown | ✓ Goodbye/Vulkan destroy/status stopped 各 1 |
| 10 | Agent 並列 disjoint scope 衝突 0 | ✓ 6 Cluster overlap 0、set/binding allocation 衝突 0 |
| 11 | 自作 bug 即 phase 2 fix (feedback_root_cause_not_dump) | ✓ 3 件全て根本修正 (revert/defer なし)、phase 2-A rename + phase 2-B guard wrap |
| 12 | Agent 誤判定検出 → re-trace (§3.2) | ✓ Cluster D Agent A 報告破棄 → Claude 再 trace |
| 13 | Agent capability limit による η-7 移管 (§3.4) | ✓ phase 2-C LINE 0:570 Avatar Eyes 移管判断記録 |
| 14 | nameless block member 固有化 (η-3/η-5/η-6 統合) | ✓ phase 2-A UnderWater (data member) + SkinSSS (`#define` alias) |
| 15 | scope refinement 5th-level (η-5 §3.2) 継承適用 | ✓ phase 1 verify → 自作 bug 3 件発見 → phase 2-A/2-B/2-C 並列 sub-bundle 内取込 |
| 16 | 過去 sub-bundle 既処理 file byte-for-byte 維持 | ✓ A1-A7/A8-recovery/B1-B3/B2-α-γ/B?-δ-η-5 全件 |
| 17 | skip list admission 範式継承 (B?-δ) | ✓ insertions-only or rename-only + outer LL_VULKAN_GLSL 不変 |
| 18 | handoff doc 別 commit (η-5 範式継承) | ✓ 本 doc は patch commit `e915af26fe` 後の別 commit |

---

## §6 設計範式継承表

### feedback memory 範式 (10 件全件適用確認)

| # | feedback memory | η-6 適用箇所 |
|---|---|---|
| 1 | feedback_doubt_self_first | Cluster D で Agent A 報告を疑い、log stage type 0x8b31 で再 trace |
| 2 | feedback_admit_unknown | phase 2-C LINE 0:570 Avatar Eyes は Agent capability limit 明示 → η-7 移管 |
| 3 | feedback_build_only_verified | cold cache launch verify を phase 2 完遂後実施、metric grep で確定 |
| 4 | feedback_falsification_as_progress | phase 1 verify で 自作 bug 3 件発見を progress として handoff doc 記録 |
| 5 | feedback_one_step_at_a_time | AYA 「OK」明示指示後 phase 単位 deploy + verify |
| 6 | feedback_no_auto_commit | AYA 「commit して」明示指示後 commit |
| 7 | feedback_no_claude_coauthor | Co-Authored-By: Claude 行を含めない (全 commit) |
| 8 | feedback_no_scope_shrink | phase 1 で 60 件全件 attempt、scope 縮小せず、移管は技術的制約のみ |
| 9 | feedback_shader_only_fast_iterate | shader file のみ編集、autobuild 不要、cp + rm shader_cache で fast iterate |
| 10 | feedback_root_cause_not_dump | 自作 bug 3 件全て根本修正、Cluster E rlvF bvec2 / Cluster C bool は技術的制約による η-7 移管区分明示 |

### prior sub-bundle 範式 (継承表)

| sub-bundle | 範式 | η-6 適用 |
|---|---|---|
| B1 §3 | shader-file 側 UBO 宣言 guard wrap | phase 1 6 Cluster 24 file + phase 2-C 10 file |
| B2-α §3.1 | GL `#else` branch byte-for-byte 不変 | 34 file 全件 |
| B3 §3.2 | metric integrity self-check | §4.4 |
| B3 §12 | literal grep metric verify | §4.1-§4.5 全 metric |
| B2-γ §3.1 | Agent 並列 disjoint scope | phase 1 Cluster B/E/F 3 並列 |
| B2-γ §3.2 | pilot 1 file で patch literal 確立 | Cluster A SMAA + Cluster C reflectionProbeF 2 pilot |
| B2-γ §3.3 | charter §3 #1 担保 | 34 file 全件 |
| B?-δ §3.1 | skip list admission | reflectionProbeF/skinSSSF/underWaterF/atmosphericsFuncs 関連等再 touch admission |
| B?-δ §3.2 | UBO body member byte-for-byte 維持 | 全 file 全 UBO body (phase 2-A は Vulkan path 内 member 名のみ rename) |
| B?-ε §3.2 | 1 source N 件 cluster 対処 | phase 2-C radianceGenF 1 file 2 program 解消 + pbropaqueV 1 file 2 program 解消 |
| B?-ζ §3.1 | shader-file 側 UBO 宣言 guard wrap | 24 file phase 1 |
| B?-η-1 §3.1 | shader-file 側 UBO 宣言 guard wrap | phase 1 全 file + phase 2-B HUD branch FrameViewProj |
| B?-η-1 §3.2 | cascade pair hypothesis | §3.5 逆方向シフト検証 |
| B?-η-1 §3.3 | Agent 並列 disjoint scope | phase 1 Cluster B/E/F 3 並列 + phase 2 並列 Agent 投入 |
| B?-η-2 (a) §3.2 | cascade pair hypothesis 汎用形 | §3.5 1:1 比例「逆」検証 |
| B?-η-3 §3.1 | scope refinement 3rd-level | §3.4 Agent capability limit による η-7 移管判断 |
| B?-η-3 §3.2 | guard macro collision detection | phase 2-A nameless block member の global scope collision detection |
| B?-η-4 §3.1 | scope refinement 4th-level (sub-bundle 内 3 phase refine) | phase 1 / phase 2-A / phase 2-B / phase 2-C 構成 |
| B?-η-4 §3.2 | nameless block member 固有化 detection 範式 | §3.3 data member 適用第 3 例 |
| B?-η-4 §3.4 | 仮説 4 件評価による真因絞り込み範式 | Cluster D Agent A 報告破棄判断 |
| B?-η-5 §3.1 | multi-root-cause 同時対処範式 (root cause 軸) | §3.1 file-cluster 軸拡張 |
| B?-η-5 §3.2 | scope refinement 5th-level (自作 bug 遡及 fix 範式) | phase 1 verify → 自作 bug 3 件発見 → phase 2-A/2-B/2-C 並列適用 |
| B?-η-5 §3.3 | Agent depth trace 投入閾値範式 | §3.2 誤判定検出補正形 + §3.4 capability limit 明示形 |
| B?-η-5 §3.4 | nameless block 範式 member-level 拡張範囲明示 (padding + data 双方) | §3.3 bare uniform vs nameless block member の collision (data member 拡張第 3 例) |

### 新規 設計範式 (§3.1-§3.5 計 5 件)

- §3.1: multi-cluster pilot + Agent 並列展開範式 (file-cluster 軸 disjoint scope、η-5 §3.1 の file-cluster 軸拡張)
- §3.2: Agent 初期 trace 誤判定検出 → log stage type literal grep 再 trace 範式 (η-5 §3.3 の誤判定検出補正形)
- §3.3: η-4 §3.2 / η-5 §3.4 nameless block 範式の bare uniform × member collision 拡張 (data member 適用第 3 例)
- §3.4: Agent capability limit による η-7 移管判断範式 (runtime preprocessed dump 不可ケース)
- §3.5: cascade pair hypothesis 汎用形 逆方向シフト検証 (patch 構造増加時)

---

## §7 risks

| # | risk | 評価 | 緩和策 |
|---|---|---|---|
| 1 | UBO body 差異 link failure exposure | ✓ 完全解消継続 (B?-η-3 達成維持) | phase 1 / 2-C 14 UBO 全件 body member 固有 名前付 |
| 2 | skip list 再 touch admission | ✓ reflectionProbeF/skinSSSF/underWaterF/atmosphericsFuncs 関連等再 touch | B?-δ admission 範式 = insertions-only or rename-only + outer LL_VULKAN_GLSL 不変 で skip list 趣旨担保 |
| 3 | cascade pair hypothesis 汎用形 逆方向シフト検証 | ✓ patch 構造増加時の逆シフト記録 | B?-η-7 以降も継続観測、main metric 主導の整合性は維持 |
| 4 | multi-cluster Cluster 境界誤選定リスク | 監視対象 | Cluster D 誤判定例 (§3.2) を範式化、§3.2 検証 step 必須 |
| 5 | 'weight' redefinition +1 新規露出 | B?-η-7 移管 | 第7層 emergence 新規、link 成立 program 集合シフト由来 |
| 6 | Agent 並列 disjoint 衝突 | ✓ 0 件 | 6 Cluster overlap 0、set/binding 14-55 連続割当衝突 0 |
| 7 | literal verify 範式 | ✓ 全 metric literal grep 確定 | B3 §12 範式継承 |
| 8 | shader_cache 302+ 維持観測点切替 | 観測継続 | -21 観測点完全解消継続維持、η-5 263 → η-6 302 +39 |
| 9 | charter §3 #1 byte-for-byte | ✓ 34 file 全件担保 | §5 #1 |
| 10 | bool / bvec2 in std140 UBO リスク | 監視対象 | Cluster C `transparent_surface` (bool) は phase 1 で wrap 済、η-6 verify で問題なし。Cluster E rlvF `bvec2` は η-7 で uint promote 必要 |
| 11 | set/binding allocation 連続割当の上限 | 監視対象 | 現状 set=3, binding=14-55 (42 bindings 使用)、Vulkan 仕様 maxDescriptorSetBoundResources 1024 内 |
| 12 | nameless block × bare uniform collision (§3.3) 範式誤伝承 | 監視対象 | 適用範囲は **nameless block member と bare uniform 同名検出時のみ**、通常 collision は §3.1 guard wrap で済 |
| 13 | Agent capability limit による移管 (§3.4) 濫用 | 監視対象 | 移管判断条件 = Agent 報告 capability limit 明示 + 24 時間 ceiling 内 sub-bundle 完了優先のみ |

---

## §8 commit history

| commit | type | scope | file 数 |
|---|---|---|---|
| `e915af26fe` | feat(r41) | η-6 patch (phase 1 + phase 2-A/2-B/2-C) | 34 file +348/-2 |
| `d6afcfaee3` | docs(r41) | η-5 完遂 handoff (起点) | 1 file +408 |
| `79246ccd7d` | feat(r41) | η-5 patch | 121 file +610/-5 |

---

## §9 file inventory (η-6 patch 34 file 内訳)

### phase 1 主指標 non-opaque 60 件対処 (24 file)

#### Cluster A pilot 1 file (Claude 自力)
- `class1/deferred/SMAA.glsl`: `SMAAParamUBO_Legacy` (set=3, binding=14)、`vec4 SMAA_RT_METRICS`

#### Cluster C pilot 1 file (Claude 自力)
- `class3/deferred/reflectionProbeF.glsl`: `ReflectionProbeUBO_Legacy` (set=3, binding=17)、`float max_probe_lod` + `bool transparent_surface`

#### Cluster B Agent 3 file
- `class1/effects/glowF.glsl`: `GlowFParamUBO_Legacy` (binding=18)
- `class1/effects/glowV.glsl`: `GlowVParamUBO_Legacy` (binding=19)
- `class1/effects/glowExtractF.glsl`: `GlowExtractFParamUBO_Legacy` (binding=20)

#### Cluster D Claude 再 trace 2 file
- `class1/deferred/pbrShadowAlphaMaskV.glsl`: `PbrShadowAlphaMaskVParamUBO_Legacy` (binding=21)、`float shadow_target_width`
- `class1/deferred/avatarAlphaShadowV.glsl`: `AvatarAlphaShadowVParamUBO_Legacy` (binding=22)、`float shadow_target_width`

#### Cluster E Agent 6 file
- `class1/deferred/dofCombineF.glsl`: binding=24
- `class1/deferred/exposureF.glsl`: binding=25
- `class1/deferred/luminanceF.glsl`: binding=26
- `class1/deferred/motionBlurF.glsl`: binding=27
- `class3/deferred/screenSpaceReflPostF.glsl`: binding=28
- `class1/deferred/skinSSSF.glsl`: binding=30 (`SkinSSSPrototypeFParamUBO_Legacy`、7 aya_* members)

#### Cluster F Agent 11 file
- `class1/interface/clipF.glsl`: binding=32
- `class1/deferred/normgenF.glsl`: binding=33
- `class1/interface/normaldebugV.glsl`: binding=37
- `class1/post/snapshotFrameF.glsl`: binding=38
- `class3/environment/underWaterF.glsl`: binding=39 (`UnderWaterFParamUBO_Legacy`、14 members)
- `class1/objects/previewV.glsl`: binding=40 (`#ifndef LL_VULKAN_GLSL` guard)
- `class1/objects/simpleColorF.glsl`: binding=41
- `class1/deferred/starsF.glsl`: binding=42
- `class1/deferred/sunDiscF.glsl`: binding=43
- `class1/deferred/moonF.glsl`: binding=44
- `class1/deferred/starsV.glsl`: binding=45

### phase 2-A nameless block member 固有化 2 file (Claude + Agent)
- `class3/environment/underWaterF.glsl`: `lightDir` → `lightDir_underwater_legacy` + `eyeVec` → `eyeVec_underwater_legacy` +29
- `class1/deferred/skinSSSF.glsl`: `aya_visual_realism_enabled` → `aya_visual_realism_enabled_skinsss_legacy` + `#define` alias +23

### phase 2-B 自作 bug 遡及 fix 1 file (Claude 自力)
- `class1/deferred/pbrShadowAlphaMaskV.glsl`: HAS_SKIN ELSE branch FrameViewProj guard wrap +18

### phase 2-C 残 non-opaque 15 件中 13 件対処 10 file (Agent)
- `class1/post/exoVignetteF.glsl`: `VignetteParamUBO_Legacy` (binding=46)
- `class1/interface/pathfindingV.glsl`: `PathfindingVParamUBO_Legacy` (binding=47)
- `class1/interface/pathfindingNoNormalV.glsl`: `PathfindingNoNormalVParamUBO_Legacy` (binding=48)
- `class1/interface/glowcombineF.glsl`: `GlowCombineFParamUBO_Legacy` (binding=49)
- `class1/interface/occlusionCubeV.glsl`: `OcclusionCubeVParamUBO_Legacy` (binding=50)
- `class1/interface/radianceGenF.glsl`: `RadianceGenFParamUBO_Legacy` (binding=51) Radiance Gen + Hero Radiance Gen 両 program 共有
- `class2/interface/irradianceGenF.glsl`: `IrradianceGenFParamUBO_Legacy` (binding=52)
- `class1/deferred/pbropaqueV.glsl`: `PbrOpaqueVParamUBO_Legacy` (binding=53) Deferred PBR Opaque + Skinned Deferred PBR Opaque 両 program 共有
- `class1/deferred/avatarF.glsl`: `AvatarFParamUBO_Legacy` (binding=54)
- `class1/deferred/velocityV.glsl`: `VelocityVParamUBO_Legacy` (binding=55)

### overlap file 確認
- `class1/deferred/skinSSSF.glsl`: phase 1 Cluster E + phase 2-A 2 重 touch (Vulkan path 内 UBO body 範囲 disjoint)
- `class3/environment/underWaterF.glsl`: phase 1 Cluster F + phase 2-A 2 重 touch (UBO body 範囲 disjoint)
- `class1/deferred/pbrShadowAlphaMaskV.glsl`: phase 1 Cluster D + phase 2-B 2 重 touch (Cluster D = bare uniform UBO wrap / phase 2-B = HAS_SKIN ELSE branch FrameViewProj、line-range disjoint)

---

## §10 次 sub-bundle B?-η-7 推奨 scope

### §10.1 確定 scope: 主指標残 + 第7層 emergence net 退行

| metric | 件数 | 主 program / source | 推定 root cause |
|---|---|---|---|
| non-opaque uniforms (残) | 3 | Avatar Eyes 0:570 + RLVa Sphere 0:1700 + reflectionmipF 0:1969 | Avatar Eyes Agent depth trace 範式 (§3.4 移管) + RLVa Sphere bvec2 std140 制限 (uint promote) + reflectionmipF 要 trace |
| nameless block ... global scope (残) | 1 | phase 2-A で解消しきれなかった 1 件 | atmosphericsFuncs.glsl AtmoExtraUBO_Legacy member とのさらなる collision (要 trace) |
| 'weight' redefinition | 1 | 第7層 emergence 新規露出 | (要 trace) |
| 'binding' | 25 | (要 trace) | SPIR-V binding 衝突または overlapping binding |
| 'location' | 27 | (要 trace) | SPIR-V location missing + overlapping location |

### §10.2 B?-η-7 着手前 trace 範式 6 ステップ (η-5 §10.2 + η-6 §3.2 範式継承)

1. **literal grep**: `grep -c "non-opaque uniforms outside a block" log` + `grep -c "nameless block" log` + `grep -c "'weight' : redefinition" log` + `grep -c "'binding'" log` + `grep -c "'location'" log`
2. **log context 抽出**: 各 program の error LINE × 件数 + stage type 番号 (0x8b30/0x8b31) を **必ず併記** (§3.2 範式 = Agent 報告に stage type 明示確認)
3. **Avatar Eyes 0:570 Agent depth trace** (§3.4 範式): runtime preprocessed source dump 取得手順を Agent に指示 (例: glslang 出力中間 file 取得)
4. **Agent 並列 disjoint scope** (η-1 §3.3 範式): 残 non-opaque 3 件 / nameless 1 件 / 'weight' 1 件 / binding 25 件 / location 27 件 を **root cause 軸** で分割 (η-5 §3.1 範式適用)
5. **Agent 報告検証 step** (§3.2 範式): stage type / source file 同定の literal grep 検証を patch 化前に必須実施
6. **set/binding allocation 連続帯 reservation** (§3.1 付随範式): η-6 binding 14-55 使用 → η-7 binding 56+ 連続割当、Cluster 別空き帯予約

### §10.3 B?-η-7 完遂後の想定 cascade exposure 第8層

- main metric non-opaque -3 + nameless -1 + 'weight' -1 全達成想定 = 第8層 emergence shader_cache 302 → 320+ 想定
- 第7層 emergence 残 (binding 25 / location 27) は B?-η-8 / B?-η-9 で sub-bundle 分割対処
- 'weight' +1 が `'weight4'` cluster (η-5 で解消) と同源かは要 trace、同源なら η-5 範式継承で即解消可能

### §10.4 第7層 emergence 系統整理 sub-bundle 分割推奨表

| sub-bundle | scope | 件数想定 | 主 patch pattern |
|---|---|---|---|
| **B?-η-7** | non-opaque 残 3 + nameless 1 + 'weight' 1 = 計 5 件 主指標 | Agent depth trace + uint promote + nameless rename + weight guard wrap | runtime preprocess dump 取得範式確立 |
| **B?-η-8** | sampler 'binding' 25 件 | sampler binding 明示 / 衝突解消 | (要 trace) |
| **B?-η-9** | 'location' 27 件 | SPIR-V location missing + overlapping location | (要 trace) |

---

## §11 観測点

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 302+ 維持観測点 | 維持中 | B?-η-7 で 320+ 想定、減少時は link 成立路径退行 |
| 2 | cascade pair hypothesis 汎用形 逆方向シフト検証 (§3.5) | 検証成立 (patch 構造増加時の逆シフト) | B?-η-7 以降も継続観測、main metric 主導整合性維持 |
| 3 | multi-cluster pilot + Agent 並列範式 (§3.1) | 適用済 | B?-η-7 では root cause 軸 (η-5 §3.1) と file-cluster 軸 (η-6 §3.1) のいずれを選ぶか着手前判断 |
| 4 | Agent 誤判定検出 → re-trace (§3.2) | 適用済 | B?-η-7 以降は Agent 報告に stage type 明示 + 検証 step 必須化 |
| 5 | nameless block × bare uniform collision 範式 (§3.3) | 適用済 | B?-η-7 残 1 件 nameless 解消で完全達成、その先は member-level 範式の applicability 監視継続 |
| 6 | Agent capability limit による η-7 移管判断 (§3.4) | 適用済 | B?-η-7 で Avatar Eyes 0:570 runtime preprocess dump 取得範式確立、Agent 投入手順を範式化 |
| 7 | set/binding allocation 連続割当範式 | binding 14-55 (42 bindings) 使用 | B?-η-7 binding 56+ 連続割当、Vulkan maxDescriptorSetBoundResources 1024 内 |
| 8 | bool / bvec2 std140 UBO リスク | Cluster C bool 問題なし、Cluster E bvec2 η-7 移管 | B?-η-7 で uint promote 範式適用 |
| 9 | scope refinement 5th-level (η-5 §3.2) 継承適用 | 適用済 (phase 2-A/2-B/2-C) | B?-η-7 でも phase 1 verify 後 自作 bug 発見時の sub-bundle 内取込 範式継続 |
| 10 | 第7層 emergence 残 (binding 25 / location 27) sub-bundle 分割 | B?-η-8/9 移管 | sub-bundle 境界 = main metric 達成後の系統別分割 |
