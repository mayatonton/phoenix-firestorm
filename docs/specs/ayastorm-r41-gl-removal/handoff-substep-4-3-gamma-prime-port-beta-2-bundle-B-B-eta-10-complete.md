# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-10 完遂 handoff

**status**: B?-η-10 完遂 → 次 sub-bundle B?-η-11 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: `e1d5ffd98c` (η-9 patch `50cb5f6713` 範式継承、AYA 「commit して」明示指示下 commit 2026-06-02)
**handoff doc commit**: 本 doc (η-9-complete `6fee4a818b` 範式継承、別 commit)
**勤続範式継承**: B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-10 scope = η-9 handoff §10 確定 **第10層 emergence `Layout location qualifier must match` 残 3 件解消** (Phase 単一、sub-bundle 細分化方針で 5 metric 中 1 系統のみに限定)。

- **Phase 単一 (1-shot ACCEPT)**: `diffuseAlphaMaskF.glsl` Vulkan path location 整合 = vary_normal 4→0 + vertex_color 2→1 + vary_texcoord0 0→2 (vary_position 3 は元々一致で変更なし)
  - `gDeferredNonIndexedDiffuseAlphaMaskProgram` = `diffuseV.glsl` + `diffuseAlphaMaskF.glsl` の V/F pair 内 location 不整合を fragment 側で整合
  - η-9 §3.1 V/F pair canonical partner 同定範式 **直接適用第 2 例**

**計**: **1 file +3/-3** (shader file のみ、C++ touch 0)

**主指標達成** (vs η-9 baseline):
- `Layout location qualifier must match` 3 → **0** ✓ **-3 / 100% 完全達成** (diffuseAlphaMaskF Vulkan path location 整合直接効果)

**既達主指標完全維持** (13 種): `non-opaque uniforms outside a block` 0 / `'binding'` 0 / `Cannot reuse block name` 0 / `'weight4' redefinition` 0 / `'weight' redefinition` 0 / `undeclared identifier` 0 / `Link failed` 0 / `'size' undeclared` 0 / `GBufferInfo redefinition struct` 0 / `'#'` 0 / `nameless block ... global scope` 0 + (η-9 完遂時 12 種 0 完全継承 + 本 sub-bundle Layout 解消で 13 種化)

**Phase 構成の特徴**: η-9 の 2-phase 構成 (Phase 1 falsification + Phase 2 軌道修正) と対比、η-10 は **Phase 1 即 ACCEPT の 1-shot 完結**。η-9 §3.1 範式適用で canonical partner を着手前 trace で確定 → falsification 不要で直接 deliverable。

---

## §2 完遂結果 metric (vs B?-η-9 baseline commit `50cb5f6713`)

| metric | η-9 baseline | η-10 (commit) | Δ vs η-9 | 判定 |
|---|---|---|---|---|
| Layout location qualifier must match | 3 | **0** | -3 | ✓ **100% 完全達成** |
| 'location' literal grep | 52 | 52 | ±0 | (Layout -3 解消は overlapping 11 + SPIR-V 41 + normalMap... 同居で literal 数不変) |
| overlapping use of location 20 | 11 | 11 | ±0 | (cascade exposure 第10層 残、η-11 移管) |
| SPIR-V requires location | 41 | 41 | ±0 | (cascade exposure 第10層 残、η-11 移管) |
| 'normalMap' redefinition | 69 | 69 | ±0 | (cascade exposure 第10層 残、η-11 移管) |
| 'depthMap' redefinition | 9 | 9 | ±0 | (cascade exposure 第10層 残、η-11 移管) |
| non-opaque uniforms outside a block | 0 | 0 | ±0 | ✓ η-8 達成維持 |
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
| parse failed | 132 | 132 | ±0 | ±0 (Layout -3 解消で第11層 emergence 微小シフト可能性、cascade pair hypothesis 観測終端) |
| shader_cache 件数 | 307 | 308 | +1 | (link 成立 program 集合シフト、η-7 305 baseline + 3) |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | ±0 | ✓ |
| Goodbye | 1 | 1 | ±0 | ✓ clean shutdown |
| Vulkan device destroyed | 1 | 1 | ±0 | ✓ clean shutdown |

**13 種 既達主指標 + Layout location qualifier 100% 完全達成** = η-10 主 scope 完全達成、残 `overlapping location 20` 11 件 + SPIR-V 必須 location 41 件 + `'normalMap'` redefinition 69 件 + `'depthMap'` redefinition 9 件 は cascade exposure 第10層 として **B?-η-11 移管**。

---

## §3 設計範式

### §3.1 新規 設計範式: handoff doc 内 canonical 記載に対する着手前再検証範式

**概要**: 前 sub-bundle handoff doc 内で同定された **canonical partner 記載** に対し、後続 sub-bundle 着手前に **literal grep で実態再確認** する範式。feedback_doubt_self_first 範式の **handoff doc 内記載に対する適用例**。

**位置付け**: η-9 §3.1 V/F pair canonical partner 同定範式の **継承時注意点**。handoff doc 内で「X が canonical」と記載されていても、後続 sub-bundle で着手時に **同 V file / F file の他 program での使用状況を再 grep** で確認 = 前 sub-bundle 範囲外の program で別 attach 関係を見逃すリスクを予防。

**Detection 手順** (2 step):
1. **handoff doc canonical 記載抽出**: 前 sub-bundle §3.X canonical partner 同定範式の適用記録 (例: η-9 §3.1 = 「diffuseF を canonical として尊重」)
2. **着手時実態再 grep**: 後続 sub-bundle の patch 対象 file が **本当に 1 program 専用か** + canonical 候補 file が **本当に複数 program 共有か** を `grep "<file>.glsl" indra/newview/llviewershadermgr.cpp` で literal 確認

**η-10 適用例**:
- 前 sub-bundle (η-9) handoff §3.1 = 「diffuseF.glsl の locations 0/1/2/3 が canonical (diffuseV.glsl L46-48 共有)」と記載
- η-10 着手時 grep 結果:
  - `diffuseV.glsl`: 3 program 共有 (gDeferredDiffuseProgram + gDeferredDiffuseAlphaMaskProgram + gDeferredNonIndexedDiffuseAlphaMaskProgram) = **真の canonical**
  - `diffuseAlphaMaskF.glsl`: 1 program 専用 (gDeferredNonIndexedDiffuseAlphaMaskProgram のみ) = **patch 対象**
- → η-9 handoff 記載の「diffuseF が canonical」は **avatarEyesV と pair 時の context** で正しいが、η-10 の Deferred Diffuse Non-Indexed Alpha Mask Shader (= diffuseV + diffuseAlphaMaskF pair) の context では **diffuseV が真の canonical**
- 結論: handoff 記載は context 依存、後続 sub-bundle で **同名概念 (canonical partner) でも attach 関係が異なれば canonical 判定が変わる** = 着手前再検証必須

**charter §3 #1 担保**: diffuseAlphaMaskF.glsl GL `#else` path `in vec3 vary_normal;` / `in vec4 vertex_color;` / `in vec2 vary_texcoord0;` 元宣言不変、Vulkan path 内 `layout(location=N)` の **数値のみ変更** (η-9 §3.1 範式継承)。

### §3.2 設計範式継承: η-9 §3.1 V/F pair canonical partner 同定範式の直接適用 第 2 例

**概要**: η-9 で確立された V/F pair canonical partner 同定範式を **第 2 例として直接適用**。範式自体に変更なし、適用 file と program のみ変更:

| 適用例 | 適用 sub-bundle | program | canonical (変更不可) | patch 対象 (1 program 専用) | 結果 |
|---|---|---|---|---|---|
| 第 1 例 | η-9 Phase 2 | gDeferredAvatarEyesProgram | diffuseF.glsl (avatarEyesV と pair 時) | avatarEyesV.glsl (4/2/0 → 0/1/2) | Layout 12→3 (-9) |
| **第 2 例** | **η-10** | **gDeferredNonIndexedDiffuseAlphaMaskProgram** | **diffuseV.glsl (3 program 共有)** | **diffuseAlphaMaskF.glsl (4/2/0 → 0/1/2)** | **Layout 3→0 (-3)** |

**注意**: 第 1 例で「diffuseF が canonical」と記載されたが、η-10 (第 2 例) では diffuseV が真の canonical = **canonical 判定は program context 依存** (§3.1 範式)。

### §3.3 設計範式継承: 1-shot ACCEPT vs 2-phase 構成の判定

**概要**: η-9 §3.2 = Phase 1 falsification + 完全 revert + Phase 2 軌道修正 範式 (2-phase 構成) と対比、η-10 = **Phase 1 即 ACCEPT の 1-shot 完結**。判定基準は **着手前 trace で canonical partner が確定的か**:

| 判定 | 着手前 trace 確定度 | Phase 構成 |
|---|---|---|
| **1-shot ACCEPT** (η-10) | canonical 同定が grep 1 step で確定 (program 列挙数で明確に区別) | Phase 1 即 deliverable、falsification 不要 |
| **2-phase 構成** (η-9) | 仮説段階で複数候補存在 (canonical partner 候補が複数 + 不確定) | Phase 1 試行 → falsification 検出 → revert + Phase 2 軌道修正 |

**η-10 適用例**:
- 着手前 trace step 3 (V/F pair 同定) で:
  - `grep "diffuseV.glsl" llviewershadermgr.cpp` → 3 hit (canonical 確定)
  - `grep "diffuseAlphaMaskF.glsl" llviewershadermgr.cpp` → 1 hit (patch 対象確定)
- → 1-shot ACCEPT 判定、Phase 1 で直接 deliverable

**handoff doc 記録価値**: 後続 sub-bundle (η-11 以降) で同様の V/F pair location mismatch に対峙する際、着手前 trace で canonical が確定的なら **1-shot で進める判断基準** として参照可能。

### §3.4 設計範式継承表 (η-1〜η-9 全件継承)

| η-N | 範式 | η-10 適用状況 |
|---|---|---|
| η-1 §3.1 | FrameViewProj guard wrap (set=0/binding=0) | η-10 適用外 (location qualifier 系) |
| η-1 §3.2 | cascade pair hypothesis (主指標 -X ↔ cascade +Y) | η-10 で観測終端 (-3 解消 vs 第11層 emergence ±0) |
| η-1 §3.3 | Agent 並列 disjoint scope | η-10 では Agent 投入 0 件 (Claude 自力 trace + patch、Phase 単一 file) |
| η-2 (a) §3.2 | UBO body member 順序 std140 完全一致 | η-10 適用外 (location qualifier 変更で UBO body 不触) |
| η-3 §3.1/§3.2 | Link failed root cause UBO body 差異解消 | η-10 適用外 (Link failed 0 維持) |
| η-4 §3.1/§3.2/§3.4 | nameless block × padding / cascade pair / UBO set=3 帯 | η-10 適用外 (location qualifier 系) |
| η-5 §3.1/§3.2/§3.3/§3.4 | multi-root-cause 同時対処 / 5th-level scope refinement / Agent depth trace / WEIGHT_LOCATION_DEFINED guard | η-10 適用外 (Phase 単一 root cause) |
| η-6 §3.1/§3.2/§3.3/§3.4/§3.5 | multi-cluster pilot / Agent 誤判定 → re-trace / nameless block × member / Agent capability limit 移管 / cascade pair 逆方向 | η-10 適用外 |
| η-7 §3.1/§3.2/§3.3/§3.4/§3.5 | bvec2→uvec2 uint promote / program name collision / 5th-level cascade emergence / nameless block × member 第 4 例 / cascade pair 順方向 | η-7 §3.2 (program name 同定) の **V/F pair 同定** への拡張は η-9 §3.1 で発展、η-10 で第 2 例 |
| η-8 §3.1/§3.2/§3.3/§3.4 | cinematic_bd override path 発見 / runtime preprocessed dump / mIndexedTextureChannels Vulkan-aware emit / UBO wrap 範式 reflectionProbeF 適用 | η-8 §3.2 (runtime dump) は η-10 では未投入 (static trace で V/F 同定可能、cinematic_bd directory audit は §10.4 で η-11+ 移管継承) |
| η-9 §3.1 | V/F pair canonical partner 同定範式 | η-10 §3.2 で **直接適用第 2 例** |
| η-9 §3.2 | Phase 1 falsification + 完全 revert + Phase 2 軌道修正 | η-10 では Phase 1 即 ACCEPT (1-shot 完結)、§3.3 で判定基準確立 |
| η-9 §3.3 | feedback_falsification_as_progress の sub-bundle 内 phase falsification 適用 | η-10 適用外 (falsification 0 件) |
| η-9 §3.4 | feedback_admit_unknown の Phase 1 → Phase 2 切替 | η-10 適用外 (1-shot 完結) |

**feedback memory 10 件全件適用確認**:
- feedback_doubt_self_first: η-10 §3.1 適用 (handoff doc canonical 記載に対する着手前再検証)
- feedback_root_cause_not_dump: diffuseAlphaMaskF を V 規約に整合 (workaround 不使用)
- feedback_shader_only_fast_iterate: shader cp + cache clear のみ (autobuild 不要)
- feedback_no_auto_commit: AYA 「commit して」明示指示下 commit
- feedback_no_claude_coauthor: commit message に Co-Authored-By 不在
- feedback_one_step_at_a_time: 着手前 trace → patch → verify → commit を順次実施
- feedback_proactive_handoff: 本 handoff doc 起草
- feedback_no_scope_shrink: η-10 scope (Layout location 3 件のみ) は AYA 明示指示 = scope shrink でなく AYA 確定 scope
- feedback_admit_unknown: η-10 では仮説 2 連続外れず (1-shot ACCEPT)、適用機会なし
- feedback_falsification_as_progress: η-10 では falsification 0 件、適用機会なし

---

## §4 cold cache launch verify 詳細

### §4.1 verify cycle 構成 (1 cycle = 1-shot ACCEPT)

**Cycle 1 (Phase 1 verify = ACCEPT)**:
- 操作: diffuseAlphaMaskF.glsl patch → shader-only fast-iterate cp + cache clear → AYA launch → 軽操作 → shutdown
- 結果:
  - `Layout location qualifier must match` 3 → **0** (-3 ✓ 100% 完全達成)
  - 13 種既達主指標 0 完全維持
  - 第10層 emergence (overlapping 11 / SPIR-V 41 / normalMap 69 / depthMap 9) 完全 ±0 (η-11 移管継承)
  - clean shutdown (Goodbye 1 + Vulkan device destroyed 1 + FATAL/SIGSEGV/Aborted 0)
- 判定: **η-10 deliverable 確定 (1-shot ACCEPT)**

### §4.2 主指標 metric integrity self-check

- `Layout location qualifier` -3 = diffuseAlphaMaskF Vulkan path location 整合直接効果 (3 件: vary_normal V=0 vs F=4 → 0/0 整合 / vertex_color V=1 vs F=2 → 1/1 整合 / vary_texcoord0 V=2 vs F=0 → 2/2 整合)
- `'location'` literal grep 52→52 = Layout match -3 + overlapping 11 + SPIR-V missing 41 + normalMap 69 + depthMap 9 同居で literal 数不変 (B3 §12 literal grep 範式 = 主指標分解必須)
- shader_cache 307→308 (+1) = diffuseAlphaMaskF link 成立で program 集合シフト (η-7 305 baseline + 3)

### §4.3 shutdown clean verify

- Cycle 1 で:
  - `Goodbye!` 1 件 + `Vulkan device destroyed` 1 件
  - FATAL/SIGSEGV/Aborted 0/0/0

---

## §5 self-verify 17 項目 all green

| # | 項目 | 状態 |
|---|---|---|
| 1 | charter §3 #1 GL path byte-for-byte 不変担保 (diffuseAlphaMaskF.glsl GL `#else` path 元宣言不変) | ✓ |
| 2 | shader file のみ touch (C++ touch 0) | ✓ |
| 3 | 主指標 `Layout location qualifier` -3 literal grep verify | ✓ |
| 4 | 既達主指標 13 種 literal grep verify 全 0 維持 | ✓ |
| 5 | parse failed integrity (132→132 ±0 + Layout -3 解消の cascade exposure 微小シフト可能性) | ✓ |
| 6 | FATAL/SIGSEGV/Aborted 0 維持 | ✓ |
| 7 | clean shutdown (Goodbye 1 + Vulkan destroy 1) | ✓ |
| 8 | shader_cache baseline 確立 (η-7 305 → η-10 308、η-11 観測点) | ✓ |
| 9 | handoff doc canonical 記載 着手前再検証範式 §3.1 適用 | ✓ |
| 10 | V/F pair canonical partner 同定範式 §3.2 直接適用第 2 例 (diffuseAlphaMaskF) | ✓ |
| 11 | 1-shot ACCEPT vs 2-phase 構成判定範式 §3.3 適用 (着手前 trace で canonical 確定 → 1-shot) | ✓ |
| 12 | feedback_doubt_self_first §3.1 適用 (handoff doc canonical 記載再検証) | ✓ |
| 13 | 過去 sub-bundle 既処理 file byte-for-byte 維持 (η-1〜η-9 全 file 不触、η-10 は diffuseAlphaMaskF.glsl 初 touch) | ✓ |
| 14 | skip list admission 範式継承 (diffuseAlphaMaskF.glsl は η-1〜η-9 全範囲外 初 touch、skip list 触れず) | ✓ |
| 15 | feedback_no_claude_coauthor 遵守 (commit message Co-Authored-By 不在) | ✓ |
| 16 | feedback_no_auto_commit 遵守 (AYA 「commit して」明示指示下 commit) | ✓ |
| 17 | handoff doc 別 commit (η-9-complete `6fee4a818b` 範式継承) | ✓ |

---

## §6 設計範式継承表

(§3.4 と内容重複のため §3.4 参照、feedback memory 10 件全件適用確認 + prior sub-bundle 30+ 件継承)

---

## §7 risks

| # | risk | mitigation |
|---|---|---|
| 1 | handoff doc canonical 記載再検証範式 §3.1 の **適用範囲過大** (全 sub-bundle で前 sub-bundle 全 §3.X 範式の再検証は工数膨張) | 着手前 trace step 3 (V/F pair 同定) の自然な拡張として grep 1 step に限定、深掘り再検証は仮説 falsification 時のみ |
| 2 | η-9 §3.1 V/F pair canonical partner 同定範式 直接適用 §3.2 の **3 program 以上で共有される F (or V) を変更不可** とする判定漏れ継承 | η-11 で同範式適用時に **grep `"<file>.glsl"` で全 program 列挙必須** (η-9 §7 #1 継承) |
| 3 | 1-shot ACCEPT vs 2-phase 構成判定範式 §3.3 の **過信** (canonical 同定が grep で確定でも overlapping 等 cascade exposure で net 退行可能性) | η-11 で 1-shot 判定時も verify cycle 1 回は必須、Phase 1 verify で第11層 emergence 観測 |
| 4 | `overlapping location 20` 11 件残存 (cinematic_bd / class3 area の location=20 重複)、**location=20 を使う既存 shader 群の audit 必要** = 単一 file 修正では完結しない可能性 | η-11 で `'location=20'` literal grep + 各 shader の attach 関係を Agent 並列 disjoint scope で trace |
| 5 | SPIR-V 必須 location 41 件 = `Layout location qualifier` 系の **未 emit 系** (location 番号未指定 in/out 宣言) = mass treatment 必要 | η-11 で **Vulkan-aware 一括 wrap script** or **C++ runtime emit Vulkan-aware 化** (η-8 §3.3 範式継承) 検討 |
| 6 | `'normalMap'` redefinition 69 件 + `'depthMap'` redefinition 9 件 = **新 root cause 系統** (η-1〜η-10 で未対処) | η-11 着手時に専用 phase 設計 (η-1 §3.3 範式 Agent 並列 disjoint scope) |
| 7 | charter §3 #1 担保が **既存 numerical literal 変更** (location 番号) で揺らぐ懸念 = **insertions-only 範式の延長** として GL `#else` 完全不変なら担保継続 | η-10 §3.2 範式で **GL `#else` path は数値含め完全不変**、Vulkan path 内 layout(location=N) 数値のみ変更 = 既範式の自然延長と整理 (η-9 §7 #7 継承) |
| 8 | shader file のみ touch 範式の **runtime emit (C++) 系 root cause 取りこぼし** = η-8 §3.3 で 1 例発生、η-10 で 0 だが η-11 で再発可能性 | η-11 着手前に **C++ shader 関連 file (llshadermgr.cpp / llglslshader.cpp / llviewershadermgr.cpp) の bare uniform / sampler emit 系列挙** を grep で先回り (η-9 §7 #9 継承) |
| 9 | cinematic_bd directory 全 .glsl audit (§10.4 = η-8 から継承、η-9・η-10 未実施) **未実施** | η-11 着手前 or 完遂後の別 phase として強く推奨 (η-8 §3.1 cinematic_bd override path 発見範式の予防的展開、η-9 §7 #10 継承) |
| 10 | shader_cache 件数 (η-7 305 baseline → η-10 308) **η-11 観測点切替** | η-11 で 308+ 維持 + 第11層 emergence 観測 |
| 11 | Phase 単一 deliverable が **single file 1 機能** で η-N sub-bundle scope として小規模 = 「sub-bundle 細分化過剰」批判可能 | feedback_no_scope_shrink 違反でなく **AYA 明示指示 (Layout location 3 件のみで進めて)** = scope shrink でなく AYA 確定 scope (η-9 §7 #12 継承) |
| 12 | feedback_no_scope_shrink 違反懸念 (handoff §10.1 5 metric 中 1 系統のみ scope 化) | AYA 「Layout location 3 件のみで進めて」明示指示下 scope 確定 (2026-06-02)、残 4 metric は η-11+ 移管 = scope shrink でなく **scope 分割** (η-9 §7 #13 継承) |
| 13 | cascade exposure 第10層 残全件 (overlapping 11 + SPIR-V 41 + normalMap 69 + depthMap 9 = 計 130 件) η-11 移管 = **sub-bundle 残量大** | η-11 着手前 trace で系統別 Phase 分割 (overlapping = 1 Phase / SPIR-V = 1 Phase / normalMap+depthMap = 1 Phase) で sub-bundle 1 件 = 1 系統 維持 |

---

## §8 commit history

| commit | sub-bundle | scope |
|---|---|---|
| `e1d5ffd98c` | B?-η-10 patch | diffuseAlphaMaskF.glsl Vulkan path location 4/2/0 → 0/1/2 整合 |
| `6fee4a818b` | B?-η-9 handoff doc | (η-9 完遂 handoff、η-10 着手境界引継) |
| `50cb5f6713` | B?-η-9 patch | avatarEyesV.glsl Vulkan path location 4/2/0 → 0/1/2 整合 (1 file +5/-3) |
| `6994eba271` | B?-η-8 handoff doc | (η-8 完遂 handoff) |
| `2542905ce0` | B?-η-8 patch | non-opaque 1 + 'binding' 25 完全達成 (3 file +60/-1 = C++ 1 + shader 2) |
| `b1e8689634` | B?-η-7 handoff doc | (η-7 完遂 handoff) |
| `874d1a7252` | B?-η-7 patch | nameless / weight redefinition / non-opaque 67% (4 file) |

---

## §9 file inventory (η-10 patch 1 file 内訳)

| Phase | file | 変更内容 | 行数 | 範式適用 |
|---|---|---|---|---|
| Phase 単一 | `indra/newview/app_settings/shaders/class1/deferred/diffuseAlphaMaskF.glsl` | Vulkan path location 4/2/0 → 0/1/2 (vary_normal + vertex_color + vary_texcoord0) | +3/-3 | §3.2 V/F pair canonical partner 同定範式 直接適用第 2 例 (diffuseV.glsl を canonical として尊重) |

**overlap file 確認**:
- diffuseAlphaMaskF.glsl: η-1〜η-9 全範囲外 (η-10 で初 touch、skip list 触れず)
- diffuseV.glsl: η-9 Phase 1 falsified 試行で参照のみ (3 program 共有 canonical 確認)、touch 0
- llshadermgr.cpp / llviewershadermgr.cpp: η-10 で参照のみ (canonical 同定 grep)、touch 0

---

## §10 次 sub-bundle B?-η-11 推奨 scope

### §10.1 確定 scope: 第10層 emergence cascade exposure 残全件 (η-10 未対処分)

| metric | η-10 末件数 | 推定 root cause | 推奨対処 |
|---|---|---|---|
| overlapping use of location 20 | 11 | cinematic_bd / class3 area の location=20 重複 (Underwater / Terrain / SMAA Blending Weights ×4 / Environment Map / Windlight Sky / Windlight Cloud / Windlight Sun / SSR Post 等) | location 番号再割当 (§3.1 範式 handoff canonical 再検証 + §3.2 V/F pair 同定で canonical 確定 + 衝突回避) |
| SPIR-V requires location | 41 | in/out 宣言の location 番号未指定 (mass) | C++ runtime emit Vulkan-aware 化 (η-8 §3.3 範式) or 一括 wrap script |
| 'normalMap' redefinition | 69 | (η-1〜η-10 未対処、新 root cause 系統) | 専用 phase (Agent 並列 disjoint scope η-1 §3.3) |
| 'depthMap' redefinition | 9 | 同上 | 同上 |
| missing #endif / parse failed | cascade 縮小傾向 | cascade pair shift | 主指標解消後の自然減 |

### §10.2 B?-η-11 着手前 trace 範式 7 ステップ (η-10 §3.1 範式追加)

1. **literal grep**: `grep -c "overlapping use of location 20" log` + `grep -c "SPIR-V requires" log` + `grep -c "'normalMap'" log` + `grep -c "'depthMap'" log` + `grep -c "Layout location qualifier" log` (η-10 達成維持確認)
2. **log context 抽出**: 各 program の error LINE × 件数 + stage type 番号 (0x8b30 vertex / 0x8b31 fragment) を **必ず併記** (η-6 §3.2 範式)
3. **V/F pair 同定** (η-9 §3.1 / η-10 §3.2 範式): location 系は **`llviewershadermgr.cpp` で `mFragmentFiles.push_back` + `mVertexFiles.push_back` 同 program 内列挙** で V/F file 確定 + **他 program での共有確認** で canonical partner 同定
4. **handoff doc canonical 記載再検証** (η-10 §3.1 範式): 前 sub-bundle (η-10) の canonical 同定記録を **着手時 grep で再確認** = canonical 判定が context 依存であることを前提
5. **Agent 並列 disjoint scope** (η-1 §3.3 範式): metric 系統別に **root cause 軸** で分割 (overlapping / SPIR-V missing / normalMap / depthMap 各別 Agent)
6. **Agent 報告検証 step** (η-6 §3.2 範式): stage type / source file 同定の literal grep 検証を patch 化前に必須実施
7. **1-shot ACCEPT vs 2-phase 構成判定** (η-10 §3.3 範式): 着手前 trace で canonical 確定なら 1-shot、不確定なら 2-phase 構成で Phase 1 falsification 許容

### §10.3 B?-η-11 完遂後の想定 cascade exposure 第11層

- overlapping 11 件解消想定 = 'location' literal grep -11 程度
- SPIR-V missing 41 件 → C++ runtime emit Vulkan-aware 化で大量解消想定
- normalMap / depthMap redefinition 78 件 → 別 sub-bundle 分割推奨 (B?-η-12 移管候補)
- shader_cache 件数: η-11 で 308+ 維持 + 第11層 emergence 観測

### §10.4 cinematic_bd 系統 systematic audit 推奨 (η-8 から継承、η-9・η-10 未実施)

η-8 §3.1 cinematic_bd override path 発見範式で shadowUtil のみ個別検出。cinematic_bd directory 内 .glsl file 全件で類似 override + 未 wrap bare uniform / 未整合 location 残存可能性。B?-η-11 着手前 or 完遂後の別 phase として:

1. `find indra/newview/app_settings/shaders/cinematic_bd -name "*.glsl"` で全件列挙
2. 各 file の bare uniform / sampler / `layout` 欠落 + location qualifier 不整合 grep audit
3. 該当 file を class1/class2/class3 版 patch と同 set/binding + location で wrap / 整合 (mutually exclusive 担保)

η-8 で発生した「想定外 cinematic_bd 起源 metric 露出」と η-11 で発生する可能性のある「cinematic_bd 起源 location mismatch」を予防的に解消。

### §10.5 runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画継承

η-9 / η-10 では未投入 (static trace で V/F 同定可能だった)。η-11 で **overlapping location 20** や **SPIR-V missing 41** の root cause file が SPIR-V helper concat 経由で static trace 不能な場合、η-8 §3.2 範式再投入 = `llglslshader.cpp` parse-fail 路径に LL_WARNS diag dump 一時追加 → sub-bundle 完遂時に **必ず除去** (η-8 baseline 完全復元)。

---

## §11 観測点

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-10 308) | ✓ η-10 で baseline 確立 | B?-η-11 で 308+ 維持 + 第11層 emergence 観測 |
| 2 | handoff doc canonical 記載 着手前再検証範式 (§3.1) 適用 | 適用済 (diffuseV/diffuseAlphaMaskF) | B?-η-11 でも前 sub-bundle canonical 記載の再 grep 必須 |
| 3 | V/F pair canonical partner 同定範式 (§3.2) 適用 第 2 例 | 適用済 (diffuseAlphaMaskF) | B?-η-11 overlapping location 20 11 件で同範式継続適用 |
| 4 | 1-shot ACCEPT vs 2-phase 構成判定範式 (§3.3) 適用 | 適用済 (1-shot) | B?-η-11 で着手前 trace 確定度で判定 |
| 5 | set/binding allocation 連続割当 (η-8 末: 14-59 + 100-103) | η-10 で binding touch なし | B?-η-11 で binding 触る場合は 60+ 連続割当開始 |
| 6 | C++ touch 0 維持 (η-8 で 1 例発生、η-9・η-10 で 0) | η-10 達成 | B?-η-11 で SPIR-V missing 41 件対処時に C++ touch 復活可能性 (η-8 §3.3 範式) |
| 7 | 既達主指標 13 種完全維持 | ✓ η-10 で達成 | B?-η-11 で達成維持 + overlapping / SPIR-V missing / normalMap / depthMap のみ scope |
| 8 | cinematic_bd directory 全 .glsl audit (§10.4) | 未実施 (η-8 から継承) | B?-η-11 着手前 or 完遂後の別 phase として推奨 |
| 9 | runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画 | η-9 / η-10 未投入 | B?-η-11 で SPIR-V helper concat 経由 location mismatch 同定時に再投入 + sub-bundle 完遂時除去 |
| 10 | cascade exposure 第10層 残全件 (overlapping 11 + SPIR-V 41 + normalMap 69 + depthMap 9 = 計 130 件) η-11 移管 | sub-bundle 残量大 | B?-η-11 着手前 trace で系統別 Phase 分割推奨 |

---

**handoff doc 完。次 sub-bundle B?-η-11 着手は本 doc § 10 推奨 scope を起点として、fresh context で実施。**
