# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-9 完遂 handoff

**status**: B?-η-9 完遂 → 次 sub-bundle B?-η-10 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: `50cb5f6713` (η-8 patch `2542905ce0` 範式継承、AYA 「OK」明示指示下 commit 2026-06-02)
**handoff doc commit**: 本 doc (η-8-complete `6994eba271` 範式継承、別 commit)
**勤続範式継承**: B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-9 scope = handoff §10 確定 **第9層 emergence 'location' cascade exposure 縮小**。**2 phase 構成** (Phase 1 falsification + Phase 2 軌道修正):

- **Phase 1 falsification** (REVERTED): `llshadermgr.cpp` `vary_texture_index` Vulkan-aware emit (`layout(location=20)`) + `diffuseF.glsl` location 0/1/2/3 → 4/2/0 試行
  - **Phase 1 verify falsified 2 件**: (a) `overlapping location 20` +25 件新規露出 (location=20 は atmosphericsVarsF.glsl `vary_AdditiveColor` 他 25 shader で既使用) + (b) "Deferred Diffuse Non-Indexed Alpha Mask Shader" link failure (V=0 / F=4 mismatch、diffuseF は diffuseV と canonical pair で 0/1/2/3 が真値だった)
  - Phase 1 全 file 完全 revert (η-8 baseline へ復元)
- **Phase 2 軌道修正** (η-9 deliverable): `avatarEyesV.glsl` のみ Vulkan path location 4/2/0 → 0/1/2 で fragment side (`diffuseF.glsl`) と整合
  - `gDeferredAvatarEyesProgram` = `avatarEyesV` + `diffuseF` の V/F pair で diffuseF 側 (canonical) を真値として尊重 → V 側を fragment と整合

**計**: **1 file +5/-3** (shader file のみ、C++ touch 0)

**主指標達成** (vs η-8 baseline):
- `Layout location qualifier must match` 12 → **3** ✓ **-9 / 67% 縮小** (avatarEyesV out location 整合直接効果)

**既達主指標完全維持** (12 種): `non-opaque uniforms outside a block` 0 / `'binding'` 0 / `Cannot reuse` 0 / `'weight4' redefinition` 0 / `'weight' redefinition` 0 / `undeclared identifier` 0 / `Link failed` 0 / `'size' undeclared` 0 / `GBufferInfo redefinition struct` 0 / `'#'` 0 / `nameless block ... global scope` 0 + (η-8 完遂時 13 種 0 完全継承)

**Phase 1 falsification 価値**: feedback_falsification_as_progress 範式 = REJECT verdict は生き残りルート絞り込みの成果。Phase 1 で「location=20 安全」「diffuseF.glsl が truth source」の 2 仮説を直接 falsify、Phase 2 で正しい方向 (avatarEyesV を V/F pair 上 fragment 側に整合) を確定。

---

## §2 完遂結果 metric (vs B?-η-8 baseline commit `2542905ce0`)

| metric | η-8 baseline | η-9 Phase 1 verify (falsified) | η-9 Phase 2 (commit) | Δ vs η-8 | 判定 |
|---|---|---|---|---|---|
| Layout location qualifier must match | 12 | (測定割愛、+25 overlapping で覆い隠れ) | **3** | -9 | ✓ **67% 縮小** |
| 'location' literal grep | 52 | (Phase 1 で +25 overlapping 露出) | 52 | ±0 | (Layout match -9 + overlapping 11 + SPIR-V missing 41 同居で literal 数不変) |
| overlapping location | 11 (baseline) | 36 (+25 退行) | 11 | ±0 | ✓ Phase 1 revert で復元 |
| non-opaque uniforms outside a block | 0 | 0 | 0 | ±0 | ✓ η-8 達成維持 |
| 'binding' | 0 | 0 | 0 | ±0 | ✓ η-8 達成維持 |
| Cannot reuse block name | 0 | 0 | 0 | ±0 | ✓ η-1 達成維持 |
| 'weight4' redefinition | 0 | 0 | 0 | ±0 | ✓ η-5 達成維持 |
| 'weight' redefinition | 0 | 0 | 0 | ±0 | ✓ η-7 達成維持 |
| undeclared identifier | 0 | 0 | 0 | ±0 | ✓ η-5 達成維持 |
| Link failed | 0 | 1 (Diffuse Non-Indexed Alpha Mask) | 0 | ±0 | ✓ Phase 1 revert で復元 |
| 'size' undeclared | 0 | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| '#' preprocessor | 0 | 0 | 0 | ±0 | ✓ ε 達成維持 |
| nameless block ... global scope | 0 | 0 | 0 | ±0 | ✓ η-7 達成維持 |
| parse failed | 132 | (Phase 1 で増加) | 132 | ±0 | ±0 (Layout -9 解消で第10層 emergence 微小シフトの可能性、cascade pair hypothesis 観測終端) |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | 0/0/0 | ±0 | ✓ |
| Goodbye | 1 | 1 | 1 | ±0 | ✓ clean shutdown |

**12 種 既達主指標 + Layout location qualifier 67% 縮小** = η-9 主 scope partial 達成、残 `Layout location qualifier` 3 件 + `overlapping location 20` 11 件 + SPIR-V 必須 location 41 件 + `'normalMap'` redefinition 69 件 + `'depthMap'` redefinition 9 件 は cascade exposure 第10層 として **B?-η-10 移管**。

---

## §3 設計範式

### §3.1 新規 設計範式: V/F pair 内 location 整合における canonical partner 同定範式

**概要**: vertex shader (V) と fragment shader (F) の出力 / 入力 location 不整合発生時、**両 file の他 program での使用状況を grep で確認** し、複数 program で共有されている方 (canonical partner) を **真値 (truth source)** として、もう片方 (本範式適用対象) を整合させる。

**位置付け**: η-1〜η-8 で確立した「shader 内既存値 byte-for-byte 不変担保」範式 (charter §3 #1) を **V/F pair 間 location qualifier 不整合** に拡張した同定範式。η-8 までは UBO wrap / sampler emit 等の **insertions-only** が主体だったが、η-9 で初めて **既存 numerical literal の変更** を扱う。

**Detection 手順** (3 step):
1. **log error 抽出**: `Layout location qualifier must match: 'X' is N or 'Y' is M` literal で V/F の location 数値の不整合を確認 + program 名と stage type を併記
2. **program 名から V/F 同定**: `llviewershadermgr.cpp` の `mFragmentFiles.push_back("<file>")` + `mVertexFiles.push_back("<file>")` で V/F file を特定
3. **canonical partner 同定**: V file / F file それぞれが **他 program でも使用されているか** を grep で確認
   - **同じ V file (or F file) を使う program が複数存在 = canonical** (変更すると他 program に波及)
   - **片方が 1 program 専用 = 変更可能側** (本範式の patch 対象)

**η-9 適用例** (Phase 2):
- log: `'vary_normal' is 0 or 'vary_normal' is 4` (Deferred Avatar Eyes Shader 0:Mismatch)
- `llviewershadermgr.cpp` L2012-2013: `gDeferredAvatarEyesProgram` = `avatarEyesV` + `diffuseF`
- 同定:
  - `diffuseF.glsl`: `gDeferredAvatarEyesProgram` (only) + 他 program でも使用 (diffuseV と pair)
  - `avatarEyesV.glsl`: `gDeferredAvatarEyesProgram` (only) ← **本 patch 対象**
- diffuseF の locations 0/1/2/3 (diffuseV.glsl L46-48 共有) が canonical
- avatarEyesV.glsl Vulkan path location 4/2/0 → **0/1/2** に整合

**charter §3 #1 担保**: avatarEyesV.glsl GL `#else` path `out vec3 vary_normal;` / `out vec4 vertex_color;` / `out vec2 vary_texcoord0;` 元宣言不変、Vulkan path 内 `layout(location=N)` の **数値のみ変更** (insertions-only 範式の自然延長)。

### §3.2 新規 設計範式: Phase 1 falsification + 完全 revert + Phase 2 軌道修正の 2 phase 構成

**概要**: 試行 patch が cold cache verify で **net 退行 (主指標解消 -X vs 第10層 emergence +Y で Y > X)** と判定された場合、Phase 1 全 file を **byte-for-byte 完全 revert** (η-N-1 baseline へ復元) → Phase 2 で **異なる patch pattern** に軌道修正。

**位置付け**: η-1 §3.2 cascade pair hypothesis 範式 (主指標解消で第N+1層 emergence 露出は通常現象) の **逆対称形** = main metric -X < 第10層 +Y で **net 退行** の場合の **revert 範式**。η-6 §3.4 Agent capability limit による移管判断範式の **sub-bundle 内軌道修正版** (移管せず同 sub-bundle 内で別 patch pattern を試行)。

**判定 3 条件** (全 AND):
1. Phase 1 verify で第10層 emergence (overlapping / link failure / 新規 'location' 等) が **主指標解消数 -X を上回る** (Y > X)
2. Phase 2 軌道修正 patch pattern が **24h ceiling 内で実装可能** (= 別 root cause / 別 file への切替で完結)
3. Phase 1 patch が **完全 isolated** (= revert で η-N-1 baseline 完全復元可能、後続 sub-bundle 依存なし)

**η-9 適用例** (Phase 1 → revert → Phase 2):
- Phase 1: llshadermgr.cpp vary_texture_index location=20 + diffuseF.glsl 0/1/2/3 → 4/2/0
  - cold cache verify: `Layout location qualifier` -9 想定 vs `overlapping location 20` +25 + Link failed +1
  - net = -9 vs +26 → **falsified**
- Phase 1 全 revert (`git diff` で η-8 baseline と完全一致確認)
- Phase 2: avatarEyesV.glsl のみ location 4/2/0 → 0/1/2 (canonical partner = diffuseF を真値として尊重)
  - cold cache verify: `Layout location qualifier` -9 ✓ / 第10層 emergence net ±0

**charter §3 #1 担保**: Phase 1 revert は GL `#else` path のみならず Vulkan path も含めた **完全 byte-for-byte 復元** (η-8 baseline `2542905ce0` と diff なし)。

### §3.3 設計範式継承: feedback_falsification_as_progress (memory) の sub-bundle 内 falsification

**概要**: feedback_falsification_as_progress = 「Phase 全候補が REJECT でも、reject 根拠を正直に積めば thesis-level pivot の論拠になる」。本範式を **sub-bundle 内 phase falsification** に適用 = Phase 1 全 file 完全 revert を **sub-bundle 失敗** ではなく **生き残りルート絞り込みの成果** として記録。

**位置付け**: η-N (各 sub-bundle) の **失敗 phase = 価値ある falsification** 範式。同 sub-bundle 内で **複数 patch pattern を試行** することで、後続 sub-bundle (η-N+1 以降) が同じ仮説で時間を浪費するリスクを予防。

**η-9 適用例**:
- Phase 1 falsification 2 件:
  - **仮説 (a)** "location=20 は vary_texture_index 専用に安全" → **REJECT** (atmosphericsVarsF.glsl `vary_AdditiveColor` 他 25 shader で既使用)
  - **仮説 (b)** "diffuseF.glsl は avatarEyesV と pair なので avatarEyesV の location 4/2/0 が truth" → **REJECT** (diffuseF は diffuseV と canonical pair = 0/1/2/3 が truth)
- 残存 surviving routes:
  - **(c)** "avatarEyesV.glsl のみ location 0/1/2 で diffuseF と整合" → **ACCEPT** (Phase 2 deliverable)

**handoff doc への記録価値**: 後続 sub-bundle (η-10 以降) で `Layout location qualifier` 残 3 件 + `overlapping location 20` 11 件を扱う際に、本 falsification 履歴 (location=20 修正は overlapping 退行リスク高) を参照して **safe location 番号選定** の絞り込みに利用可能。

### §3.4 設計範式継承: feedback_admit_unknown (memory) の Phase 1 → Phase 2 切替

**概要**: feedback_admit_unknown = 「仮説 2 連続外れたら推論止めて log/canary/bisect で実データ取得に切替」。本範式を **Phase 1 falsification 検出時の Phase 2 設計** に適用 = Phase 1 verify log の **直接観測 data** で Phase 2 軌道修正方向を確定。

**η-9 適用例**:
- Phase 1 verify log 観測 data 3 件:
  - (1) `Layout location qualifier must match: 'vary_normal' is 0 or 'vary_normal' is 4` (Deferred Diffuse Non-Indexed Alpha Mask Shader) → V=0 / F=4 mismatch 直接観測 = **diffuseV (0) が truth, diffuseF (4) が誤り**
  - (2) `overlapping location 20` literal grep 件名列挙 (atmosphericsVarsF.glsl `vary_AdditiveColor` 他) = location=20 安全仮説の直接 falsification
  - (3) `Deferred Avatar Eyes Shader` の Layout match error 残存 = avatarEyesV / diffuseF mismatch 別途存在

- Phase 2 軌道修正 = (1)(3) を統合 → **diffuseF を truth (canonical) として尊重、avatarEyesV を fragment 側に整合**

### §3.5 設計範式継承: η-1〜η-8 全件継承表

| η-N | 範式 | η-9 適用状況 |
|---|---|---|
| η-1 §3.1 | FrameViewProj guard wrap (set=0/binding=0) | avatarEyesV 既 wrap (η-5)、η-9 で同 file 別 line-range 第 2 touch (B?-δ admission) |
| η-1 §3.2 | cascade pair hypothesis (主指標 -X ↔ cascade +Y) | η-9 で逆対称 (Phase 1 -9 vs +26 net 退行 → revert) で観測終端 |
| η-1 §3.3 | Agent 並列 disjoint scope | η-9 では Agent 投入 0 件 (Claude 自力 trace + patch、Phase 2 単一 file) |
| η-2 (a) §3.2 | UBO body member 順序 std140 完全一致 | η-9 適用外 (location qualifier 変更で UBO body 不触) |
| η-3 §3.1/§3.2 | Link failed root cause UBO body 差異解消 | η-9 適用 (Phase 1 で Link failed 1 件露出 → revert で解消) |
| η-4 §3.1/§3.2/§3.4 | nameless block × padding / cascade pair / UBO set=3 帯 | η-9 適用外 (location qualifier 系) |
| η-5 §3.1/§3.2/§3.3/§3.4 | multi-root-cause 同時対処 / 5th-level scope refinement / Agent depth trace / WEIGHT_LOCATION_DEFINED guard | η-9 適用外 (Phase 2 単一 root cause) |
| η-6 §3.1/§3.2/§3.3/§3.4/§3.5 | multi-cluster pilot / Agent 誤判定 → re-trace / nameless block × member / Agent capability limit 移管 / cascade pair 逆方向 | η-6 §3.4 (移管判断) の **sub-bundle 内軌道修正版** として η-9 §3.2 で発展 |
| η-7 §3.1/§3.2/§3.3/§3.4/§3.5 | bvec2→uvec2 uint promote / program name collision / 5th-level cascade emergence / nameless block × member 第 4 例 / cascade pair 順方向 | η-7 §3.2 (program name 同定) の **V/F pair 同定** への拡張として η-9 §3.1 で発展 |
| η-8 §3.1/§3.2/§3.3/§3.4 | cinematic_bd override path 発見 / runtime preprocessed dump / mIndexedTextureChannels Vulkan-aware emit / UBO wrap 範式 reflectionProbeF 適用 | η-8 §3.2 (runtime dump) は η-9 では未投入 (static trace で V/F 同定可能だった、cinematic_bd directory audit は §10.4 で η-10+ 移管) |

**feedback memory 10 件全件適用確認**:
- feedback_falsification_as_progress: η-9 §3.3 適用 (Phase 1 falsification 2 件記録)
- feedback_admit_unknown: η-9 §3.4 適用 (Phase 1 → Phase 2 切替判定)
- feedback_doubt_self_first: Phase 3 unification 仮説検証で各 V file 独立確認 = Phase 3 計画破棄
- feedback_root_cause_not_dump: avatarEyesV のみ touch + diffuseF を truth として尊重 (workaround 不使用)
- feedback_shader_only_fast_iterate: Phase 1 revert + Phase 2 全工程で shader cp + cache clear (autobuild 不要)
- feedback_no_auto_commit: AYA 「OK」明示指示下 commit
- feedback_no_claude_coauthor: commit message に Co-Authored-By 不在
- feedback_one_step_at_a_time: Phase 1 verify → Phase 2 verify を順次実施
- feedback_proactive_handoff: 本 handoff doc 起草
- feedback_no_scope_shrink: η-9 scope (第9層 'location' cascade exposure 縮小) を勝手に絞らず 12→3 partial 達成として正直記録

---

## §4 cold cache launch verify 詳細

### §4.1 verify cycle 構成 (2 cycle)

**Cycle 1 (Phase 1 verify = falsification 検出)**:
- 操作: llshadermgr.cpp + diffuseF.glsl patch → autobuild → install → cache clear → AYA launch
- 結果:
  - `Layout location qualifier` 12 → (overlapping +25 で覆い隠れ)
  - `overlapping location 20` 11 → 36 (+25 退行)
  - `Link failed` 0 → 1 (Deferred Diffuse Non-Indexed Alpha Mask Shader = V=0 / F=4 mismatch)
- 判定: net 退行 → Phase 1 全 file revert

**Cycle 2 (Phase 2 verify = 軌道修正)**:
- 操作: Phase 1 全 revert + avatarEyesV.glsl のみ location 4/2/0 → 0/1/2 → shader-only fast-iterate cp + cache clear → AYA launch
- 結果:
  - `Layout location qualifier` 12 → **3** (-9 ✓)
  - `overlapping location 20` 36 → 11 (Phase 1 退行解消)
  - `Link failed` 1 → 0 (Phase 1 退行解消)
  - 12 種既達主指標 0 完全維持
- 判定: η-9 deliverable 確定

### §4.2 主指標 metric integrity self-check

- `Layout location qualifier` -9 = avatarEyesV out location 整合直接効果 (3 件: vary_normal / vertex_color / vary_texcoord0)
- 残 3 件 = SPIR-V helper concat 経由の location mismatch 別 program (η-10 trace 必須)
- `'location'` literal grep 52→52 = Layout match -9 + overlapping 11 + SPIR-V missing 41 同居で literal 数不変 (B3 §12 literal grep 範式 = 主指標分解必須)

### §4.3 shutdown clean verify

- Phase 1 verify cycle + Phase 2 verify cycle 両 cycle で:
  - `Goodbye!` 1 件 + `Vulkan device destroyed` 1 件 + `Vulkan instance destroyed` 1 件 + `status: stopped` 1 件
  - FATAL/SIGSEGV/Aborted 0/0/0

---

## §5 self-verify 18 項目 all green

| # | 項目 | 状態 |
|---|---|---|
| 1 | charter §3 #1 GL path byte-for-byte 不変担保 (avatarEyesV.glsl GL `#else` path 元宣言不変) | ✓ |
| 2 | shader file のみ touch (C++ touch 0 = llshadermgr.cpp Phase 1 全 revert で η-8 baseline 完全一致) | ✓ |
| 3 | 主指標 `Layout location qualifier` -9 literal grep verify | ✓ |
| 4 | 既達主指標 12 種 literal grep verify 全 0 維持 | ✓ |
| 5 | Phase 1 falsification 完全 revert (η-8 baseline `2542905ce0` と diff なし、llshadermgr.cpp + diffuseF.glsl 該当範囲) | ✓ |
| 6 | parse failed integrity (132→132 ±0 + Layout -9 解消の cascade exposure 微小シフト可能性) | ✓ |
| 7 | FATAL/SIGSEGV/Aborted 0 維持 | ✓ |
| 8 | clean shutdown (Goodbye 1 + Vulkan destroy 1 + status stopped 1) | ✓ |
| 9 | V/F pair canonical partner 同定範式 §3.1 適用 (avatarEyesV を fragment 側 = diffuseF に整合) | ✓ |
| 10 | Phase 1 falsification + Phase 2 軌道修正 範式 §3.2 適用 | ✓ |
| 11 | feedback_falsification_as_progress §3.3 適用 (Phase 1 全 REJECT を生き残りルート絞り込み成果として記録) | ✓ |
| 12 | feedback_admit_unknown §3.4 適用 (Phase 1 verify log 直接観測 data で Phase 2 軌道修正) | ✓ |
| 13 | feedback_doubt_self_first 適用 (Phase 3 unification 仮説 = majority 4/2/0/3 を grep 検証 → 各 V file location 独立 → 計画破棄) | ✓ |
| 14 | 過去 sub-bundle 既処理 file byte-for-byte 維持 (η-1〜η-8 全 file 不触、η-9 は avatarEyesV.glsl のみ第 2 touch) | ✓ |
| 15 | skip list admission 範式継承 (avatarEyesV は η-5 既 touch、η-9 は同 file 別 line-range = B?-δ admission 範式) | ✓ |
| 16 | feedback_no_claude_coauthor 遵守 (commit message Co-Authored-By 不在) | ✓ |
| 17 | feedback_no_auto_commit 遵守 (AYA 「OK」明示指示下 commit) | ✓ |
| 18 | handoff doc 別 commit (η-8-complete `6994eba271` 範式継承) | ✓ |

---

## §6 設計範式継承表

(§3.5 と内容重複のため §3.5 参照、feedback memory 10 件全件適用確認 + prior sub-bundle 30+ 件継承)

---

## §7 risks

| # | risk | mitigation |
|---|---|---|
| 1 | V/F pair canonical partner 同定範式 §3.1 の **3 program 以上で共有される F (or V) を変更不可** とする判定漏れ | η-10 で同範式適用時に **grep `\"<file>.glsl\"` で全 program 列挙必須** |
| 2 | Phase 1 falsification + Phase 2 軌道修正 §3.2 の **24h ceiling 超過リスク** (Phase 1 失敗 → Phase 2 設計時間消費) | η-9 では Phase 1 revert + Phase 2 implementation を 1 verify cycle (< 1h) で完了、後続 sub-bundle でも同 ceiling 担保 |
| 3 | `Layout location qualifier` 残 3 件 (Avatar Eyes 等の SPIR-V helper concat 経由 mismatch) は **static trace 不能** = η-10 で runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入必要 | η-10 着手前に **diag dump 再追加** 計画 + sub-bundle 完遂時に必ず除去 |
| 4 | `overlapping location 20` 11 件残存は **location=20 を使う既存 shader 群の audit 必要** = 単一 file 修正では完結しない可能性 | η-10 で `'location=20'` literal grep + 各 shader の attach 関係を Agent 並列 disjoint scope で trace |
| 5 | SPIR-V 必須 location 41 件 = `Layout location qualifier` 系の **未 emit 系** (location 番号未指定 in/out 宣言) = mass treatment 必要 | η-10 で **Vulkan-aware 一括 wrap script** or **C++ runtime emit Vulkan-aware 化** (η-8 §3.3 範式継承) 検討 |
| 6 | `'normalMap'` redefinition 69 件 + `'depthMap'` redefinition 9 件 = **新 root cause 系統** (η-1〜η-8 で未対処) | η-10 着手時に専用 phase 設計 (η-1 §3.3 範式 Agent 並列 disjoint scope) |
| 7 | charter §3 #1 担保が **既存 numerical literal 変更** (location 番号) で揺らぐ懸念 = **insertions-only 範式の延長** として GL `#else` 完全不変なら担保継続 | η-9 §3.1 範式で **GL `#else` path は数値含め完全不変**、Vulkan path 内 layout(location=N) 数値のみ変更 = 既範式の自然延長と整理 |
| 8 | feedback_falsification_as_progress §3.3 の **Phase 1 falsification 過剰展開** (revert を多用すると効率低下) | η-9 では Phase 1 verify cycle 1 回 + Phase 2 で完結、後続 sub-bundle でも 1 sub-bundle 内 falsification は 1-2 回まで |
| 9 | shader file のみ touch 範式の **runtime emit (C++) 系 root cause 取りこぼし** = η-8 §3.3 で 1 例発生、η-9 では C++ touch 0 だが η-10 で再発可能性 | η-10 着手前に **C++ shader 関連 file (llshadermgr.cpp / llglslshader.cpp / llviewershadermgr.cpp) の bare uniform / sampler emit 系列挙** を grep で先回り |
| 10 | cinematic_bd directory 全 .glsl audit (§10.4 = η-8 から継承) **未実施** | η-10 着手前 or 完遂後の別 phase として強く推奨 (η-8 §3.1 cinematic_bd override path 発見範式の予防的展開) |
| 11 | shader_cache 件数 (η-7 305 baseline) **次 verify 時測定** = η-9 では未取得 (verify cycle 2 件で測定割愛、Phase 1 falsification 影響で baseline 不明確) | η-10 着手時に **shader_cache 件数 baseline 確立** 必須 |
| 12 | Phase 2 deliverable が **single file 1 機能** で η-N sub-bundle scope として小規模 = 「sub-bundle 細分化過剰」批判可能 | Phase 1 falsification 履歴を含めると **2 patch pattern 試行 + 1 revert + 1 Phase 2 軌道修正** で実質的に 1 sub-bundle 相当の trace 工数 |
| 13 | feedback_no_scope_shrink 違反懸念 (Layout 12→3 = 67% partial 達成、100% 達成しないまま sub-bundle 終端) | sub-bundle 細分化方針 (η-1〜η-8 範式継承) で 1 sub-bundle 1 root cause focus、残 3 件 + overlapping + SPIR-V missing 41 + redefinition は η-10 別 phase 移管 = scope shrink でなく **scope 分割** |

---

## §8 commit history

| commit | sub-bundle | scope |
|---|---|---|
| `50cb5f6713` | B?-η-9 patch | avatarEyesV.glsl Vulkan path location 4/2/0 → 0/1/2 整合 |
| `6994eba271` | B?-η-8 handoff doc | (η-8 完遂 handoff、η-9 着手境界引継) |
| `2542905ce0` | B?-η-8 patch | non-opaque 1 + 'binding' 25 完全達成 (3 file +60/-1 = C++ 1 + shader 2) |
| `b1e8689634` | B?-η-7 handoff doc | (η-7 完遂 handoff) |
| `874d1a7252` | B?-η-7 patch | nameless / weight redefinition / non-opaque 67% (4 file) |
| `fe757ea624` | B?-η-6 handoff doc | (η-6 完遂 handoff) |
| `e915af26fe` | B?-η-6 patch | non-opaque 60→3 / multi-cluster pilot (34 file) |

---

## §9 file inventory (η-9 patch 1 file 内訳)

| Phase | file | 変更内容 | 行数 | 範式適用 |
|---|---|---|---|---|
| Phase 1 (REVERTED) | `indra/llrender/llshadermgr.cpp` | `vary_texture_index` Vulkan-aware emit (location=20) | (revert 後 +0/-0) | falsified by `overlapping location 20` +25 |
| Phase 1 (REVERTED) | `indra/newview/app_settings/shaders/class1/deferred/diffuseF.glsl` | Vulkan path location 0/1/2/3 → 4/2/0 | (revert 後 +0/-0) | falsified by `Link failed` +1 (V=0/F=4 mismatch) |
| Phase 2 (commit) | `indra/newview/app_settings/shaders/class1/deferred/avatarEyesV.glsl` | Vulkan path location 4/2/0 → 0/1/2 + comment +2 行 | +5/-3 | §3.1 V/F pair canonical partner 同定範式適用 (diffuseF を canonical として尊重) |

**overlap file 確認**:
- avatarEyesV.glsl: B?-η-5 で FrameViewProj guard wrap 既 touch (line 26-44 範囲) + η-9 で別 line-range (out location 範囲 = line 85-104) **第 2 touch** = B?-δ admission 範式継承 (line-range disjoint で AYAstorm 改変保護担保)
- llshadermgr.cpp: η-8 で mIndexedTextureChannels Vulkan-aware emit 既 touch、η-9 Phase 1 で別 emit (vary_texture_index) 試行 → revert で η-8 baseline と完全一致
- diffuseF.glsl: η-1〜η-8 全範囲外 (η-9 で初 touch 試行 → revert で η-8 baseline 完全一致)

---

## §10 次 sub-bundle B?-η-10 推奨 scope

### §10.1 確定 scope: 第10層 emergence cascade exposure

| metric | η-9 末件数 | 推定 root cause | 推奨対処 |
|---|---|---|---|
| Layout location qualifier must match | 3 | SPIR-V helper concat 経由 location mismatch / V/F pair 別 program | §3.1 範式 + (Avatar Eyes 等) runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入 |
| overlapping location 20 | 11 | cinematic_bd / class3 area の location=20 重複 | location 番号再割当 (§3.1 範式で canonical 同定 + 衝突回避) |
| SPIR-V 必須 location missing | 41 | in/out 宣言の location 番号未指定 (mass) | C++ runtime emit Vulkan-aware 化 (η-8 §3.3 範式) or 一括 wrap script |
| 'normalMap' redefinition | 69 | (η-1〜η-8 未対処、新 root cause 系統) | 専用 phase (Agent 並列 disjoke scope η-1 §3.3) |
| 'depthMap' redefinition | 9 | 同上 | 同上 |
| missing #endif / parse failed | cascade 縮小傾向 | cascade pair shift | 主指標解消後の自然減 |

### §10.2 B?-η-10 着手前 trace 範式 6 ステップ (η-8 §10.2 範式継承 + V/F pair 同定追加)

1. **literal grep**: `grep -c "'Layout location qualifier'" log` + `grep -c "overlapping location" log` + `grep -c "'normalMap'" log` + `grep -c "'depthMap'" log` (η-9 達成維持確認)
2. **log context 抽出**: 各 program の error LINE × 件数 + stage type 番号 (0x8b30 vertex / 0x8b31 fragment) を **必ず併記** (η-6 §3.2 範式)
3. **V/F pair 同定** (η-9 §3.1 範式): Layout location qualifier 系は **`llviewershadermgr.cpp` で `mFragmentFiles.push_back` + `mVertexFiles.push_back` 同 program 内列挙** で V/F file 確定 + **他 program での共有確認** で canonical partner 同定
4. **Agent 並列 disjoint scope** (η-1 §3.3 範式): metric 系統別に **root cause 軸** で分割 (Layout match / overlapping / SPIR-V missing / normalMap / depthMap 各別 Agent)
5. **Agent 報告検証 step** (η-6 §3.2 範式): stage type / source file 同定の literal grep 検証を patch 化前に必須実施
6. **set/binding allocation 連続帯 reservation**: η-8 binding=59 + 100-103 使用 + η-9 binding touch なし → η-10 binding=60+ 連続割当 (= 60-99 帯予約継続)

### §10.3 B?-η-10 完遂後の想定 cascade exposure 第11層

- Layout location 3 件 + overlapping 11 件解消想定 = 'location' literal grep -14 程度
- SPIR-V missing 41 件 → C++ runtime emit Vulkan-aware 化で大量解消想定
- normalMap / depthMap redefinition 78 件 → 別 sub-bundle 分割推奨 (B?-η-11 移管候補)
- shader_cache 件数: η-10 で baseline 確立 + 第11層 emergence 観測

### §10.4 cinematic_bd 系統 systematic audit 推奨 (η-8 から継承、η-9 未実施)

η-8 §3.1 cinematic_bd override path 発見範式で shadowUtil のみ個別検出。cinematic_bd directory 内 .glsl file 全件で類似 override + 未 wrap bare uniform / 未整合 location 残存可能性。B?-η-10 着手前 or 完遂後の別 phase として:

1. `find indra/newview/app_settings/shaders/cinematic_bd -name "*.glsl"` で全件列挙
2. 各 file の bare uniform / sampler / `layout` 欠落 + location qualifier 不整合 grep audit
3. 該当 file を class1/class2/class3 版 patch と同 set/binding + location で wrap / 整合 (mutually exclusive 担保)

η-8 で発生した「想定外 cinematic_bd 起源 metric 露出」と η-9 で発生する可能性のある「cinematic_bd 起源 location mismatch」を予防的に解消。

### §10.5 Phase 3 unification 計画破棄の記録

η-9 着手検討中に Phase 3 案として **「majority convention 4/2/0/3 統一」** を検討したが、各 V file location 規約完全独立 (benchmarkV=20 単独 / shadowAlphaMaskV=15/8/2/0 / treeV=2/0 / diffuseNoColorV=2/0 等) を grep で確認 = **majority 不在** = 計画破棄。

η-10 以降で同様の「全 file 統一」発想を出した場合、本記録を参照して **各 program V/F pair 単位の整合範式 (§3.1)** を優先選択。

---

## §11 観測点

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-9 未測定) | (η-10 verify 時必須測定) | B?-η-10 着手時に baseline 確立 |
| 2 | V/F pair canonical partner 同定範式 (§3.1) 適用 | 適用済 (avatarEyesV) | B?-η-10 Layout location qualifier 残 3 件で同範式継続適用 |
| 3 | Phase 1 falsification + Phase 2 軌道修正 範式 (§3.2) 適用 | 適用済 (η-9 sub-bundle 内 2 phase) | B?-η-10 で同様の試行 patch falsification 発生時に再適用 |
| 4 | feedback_falsification_as_progress (§3.3) 適用 | 適用済 (Phase 1 全 REJECT 記録) | B?-η-10 で複数 patch pattern 試行時の判定基準 |
| 5 | feedback_admit_unknown (§3.4) 適用 | 適用済 (Phase 1 verify log → Phase 2 軌道修正) | B?-η-10 で仮説 2 連続 falsified 時の data 切替判定 |
| 6 | set/binding allocation 連続割当 (η-8 末: 14-59 + 100-103) | η-9 で binding touch なし | B?-η-10 binding=60+ 連続割当開始 |
| 7 | C++ touch 0 維持 (η-8 で 1 例発生、η-9 で 0) | η-9 達成 (Phase 1 revert 後) | B?-η-10 で runtime emit 系 root cause 出現時に C++ touch 復活可能性 |
| 8 | 既達主指標 12 種完全維持 | ✓ η-9 で達成 | B?-η-10 で達成維持 + Layout location / overlapping / SPIR-V missing / normalMap / depthMap のみ scope |
| 9 | cinematic_bd directory 全 .glsl audit (§10.4) | 未実施 (η-8 から継承) | B?-η-10 着手前 or 完遂後の別 phase として推奨 |
| 10 | runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画 | η-9 では未投入 | B?-η-10 で Avatar Eyes 等 SPIR-V helper concat 経由 location mismatch 同定時に再投入 + sub-bundle 完遂時除去 |

---

**handoff doc 完。次 sub-bundle B?-η-10 着手は本 doc § 10 推奨 scope を起点として、fresh context で実施。**
