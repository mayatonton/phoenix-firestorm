# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-2 (a) 完遂 → 次 sub-bundle (B?-η-3 仮称) 着手境界 handoff (2026-06-01)

**parent commit**: `b67b91152a` (B?-η-2 (a) patch、本 handoff の直接 parent) / `bf9ae4950c` (B?-η-1 patch 範式継承元) / `92e3550dca` (B?-η-1-complete handoff doc commit)
**HEAD**: `b67b91152a` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: B?-η-2 の **(a) scope のみ** 完遂状態 + 次 sub-bundle (推奨 B?-η-3 仮称 = (b)+(c) cascade root cause 3 種別整理 scope + 第6層 emergence 6 種+α 整理) 着手判断境界 を fresh context 引継 用に確定する doc-only handoff。B?-η-1-complete `92e3550dca` 範式継承。**handoff §10 (b) 想定 falsify による scope refinement 範式 1 件を B?-η-2 (a) で新規確立** (= `feedback_falsification_as_progress` 範式の handoff-level 適用、(b)+(c) 想定 = preprocessor balance 不整合 vs 実態 = cascade root cause 3 種類 / 個別 fix 範式異なる、scope shrink でなく scope refinement)。`'Cannot reuse block name within the same interface: uniform'` **20→0 (-20 / 100% 完全解消)** ✓ 主指標完全達成。25 file +100/-0 insertions-only、shader file のみ編集 (C++ touch 0)、Agent (general-purpose) 3 件 parallel disjoint scope + pilot 1 file (Claude 自力 = sumLightsV.glsl)。AYA 「OK」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-B scope 第七 sub-bundle B?-η の sub-step 2 の **(a) scope のみ** (= FrameLights UBO 宣言 shader-file 側 guard wrap) を **25 file +100/-0 insertions-only** で完遂した状態を確定し、次 sub-bundle 着手境界を fresh context に引継ぐ。B?-η-1 (commit `bf9ae4950c` = FrameAtmosphere + PerDrawUBO guard wrap 53 file) の cascade exposure 第5層で露出していた **`Cannot reuse block name within the same interface: uniform` 20 件 (FrameLights LINE 918/993/1538 cluster)** を、**FrameLights UBO 宣言 (25 file) を `#ifndef FRAME_LIGHTS_DEFINED` guard wrap 4 strdup 追加/file** で **20→0 (100% 完全解消)** ✓ 主指標達成。同時に cascade pair hypothesis を η-2 で再検証 (`missing #endif` -16 ≈ 1:1 対称性、FrameLights cascade pair 20 件 -20 自動消滅 + 第6層 emergence 4 件 +4 で整合)。

本 sub-bundle は **handoff §10 想定 (b) falsify** の主検証:
- B?-η-1-complete handoff §10 では (b) `missing #endif` 9 件想定対処 = 「個別 file 特定 + outer `#ifdef ... #endif` バランス検証 + 不整合修正」と記述
- 本 η-2 (a) 完遂後 log で `missing #endif` 残 13 件を context 抽出 → **全 9 件 (handoff §10 列挙 LINE と一致) は cascade、root cause = non-opaque uniforms 5 件 + undeclared identifier 3 件 + 'weight4'/'weight' redefinition 2 件 = 3 種類異なる対処範式必要**
- `#ifdef`/`#endif` balance 不整合は **皆無** = handoff §10 想定 falsify
- `feedback_falsification_as_progress` 範式の handoff-level 適用 = scope refinement で (b)+(c) を次 sub-bundle に振替、(a) のみ commit で clean に閉じ、次 sub-bundle で 3 種類対処範式を整理し直す

本 sub-bundle は **B?-η-1 §3.1 範式の直接継承** の実例 (η-1 と同形):
- η-1 = FrameAtmosphere + PerDrawUBO UBO 宣言 shader-file 側 guard wrap (多重定義 + 同名衝突 / 53 file)
- 本 η-2 (a) = FrameLights UBO 宣言 shader-file 側 guard wrap (多重定義 + 同名衝突 / 25 file)
- guard name 規約 = SCREAMING_SNAKE_CASE + `_DEFINED` 接尾辞 (η-1 と同形)
- patch literal 4 行/file insertions-only (η-1 と同形)
- 構造的差異なし

本 sub-bundle は **η-1 §3.3 範式の直接継承** の実例 (Agent 並列 disjoint scope):
- 24 file 残 (pilot 1 file 除外) を 3 Agent × 8 file で完全 disjoint 分割
- Agent A 8 file = class1/deferred 系
- Agent B 8 file = class2 + class3/deferred 系
- Agent C 8 file = 環境/lighting/cinematic_bd 系
- pilot 1 file (Claude 自力 sumLightsV.glsl) で patch literal 確立
- Agent 報告と self literal verify 併走 (`git diff --stat` 合計 25 files / 100 insertions / 0 deletions と計算値 4×25 = 100 一致確認)

本 sub-bundle は **`feedback_doubt_self_first` + `feedback_admit_unknown` 範式適用** の実例:
- Agent C 報告: "pbrmetallicroughnessF.glsl では FRAME_ATMOSPHERE_DEFINED guard が grep で見つからず = 本 file η-1 で未 touch を確認"
- Claude 側 self verify: η-1 commit `bf9ae4950c` で本 file の実 diff を `git show` で抽出 → 実態 = η-1 で PerDrawUBO guard 適用済 (line 158-161)、FrameAtmosphere 不在 (本 file は FrameAtmosphere 持たない file = PerDrawUBO のみ持つ shader)、Agent C の interpretation は誤り但し patch 適用 OK = 整合性担保

---

## §2 B?-η-2 (a) 完遂 status

| 項目 | 値 |
|---|---|
| Scope | (a) = FrameLights UBO 宣言 shader-file 側 guard wrap、`#ifndef FRAME_LIGHTS_DEFINED` / `#define FRAME_LIGHTS_DEFINED 1` / `#endif` で wrap、4 行/file |
| 修正 file 数 | **25 file** (η-1 既編集 13 + η-2 fresh 12) |
| 変更行数 | **+100 / -0** (insertions-only、4×25 = 100) |
| 修正範囲 | 25 shader file 全 `#ifdef LL_VULKAN_GLSL` branch 内側、UBO body 9 member byte-for-byte 不変 + guard 前後 3 + 1 行追加 (comment 1 行 + `#ifndef` 1 行 + `#define` 1 行 + 末 `#endif` 1 行) |
| Agent 投入 | **3 件 parallel disjoint scope** (Agent A general-purpose = 8 file class1/deferred 系 / Agent B general-purpose = 8 file class2 + class3/deferred 系 / Agent C general-purpose = 8 file 環境/lighting/cinematic_bd 系) + Claude 自力 pilot 1 file (sumLightsV.glsl) |
| shader file 触り | **25 件** (本 sub-bundle は shader file のみ、C++ touch 0、A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ/B?-ε/B?-ζ/B?-η-1 既処理 file 含む再 touch は §2.1 で個別検証) |
| AYAstorm 改変保全 | **GL path 全不変** (`#else` branch literal 全 25 file 不変、UBO body 9 member literal 全 25 file 不変、outer `#ifdef LL_VULKAN_GLSL ... #endif` 全 25 file 不変、inner guard は LL_VULKAN_GLSL branch 内側のみ、既存 η-1 patch (FrameAtmosphere/PerDrawUBO guard、13 file) 全不変、charter §3 #1 acceptance) |
| skip list 13 + A2 拡張 skip 2 + 5 V skip | 13 file 中 3 file (`class1/windlight/atmosphericsFuncs.glsl` / `class1/deferred/godraysF.glsl` / `class3/deferred/volumetricLightF.glsl`) + cinematic_bd 1 file (`cinematic_bd/class1/deferred/shadowUtil.glsl`) を再 touch = **B?-δ admission 範式継承** (§2.2 参照) |
| AYA cold cache launch verify | **PASS** (起動成立 14:41 + cache 再生成 249 shaderbin = B?-η-1 baseline 247 を 2 件上回り + clean shutdown 14:42 + Goodbye! 1 件 + Vulkan device/instance destroyed 2 件 + status: stopped 1 件 + 実 FATAL/SIGSEGV/Aborted 0 件) |
| commit | `b67b91152a` (AYA 「OK」明示指示下 2026-06-01) |
| metric vs B?-η-1 baseline `Cannot reuse block name within the same interface` | **-20 ✓ B?-η-2 (a) 直接効果 100% 完全達成** (20→0) |
| metric vs B?-η-1 baseline `missing #endif` | **-16 cascade pair hypothesis 再検証** (29→13、FrameLights cascade pair 20 件 -20 自動解消 + 第6層 emergence 4 件 +4) |
| metric vs B?-η-1 baseline `link failed` | +1 (9→10 第6層 emergence) |
| metric vs B?-η-1 baseline non-opaque uniforms | +3 (35→38 第6層 emergence) |
| metric vs B?-η-1 baseline parse failed | -3 (191→188 cascade 内訳シフト) |
| metric vs B?-η-1 baseline redefinition (全体) | +2 (2→4 第6層 emergence = TerrainMix redefinition struct × 2、既存 `weight4`/`weight` 2 件は (c) scope 残存) |
| metric vs B?-η-1 baseline `GBufferInfo` redefinition struct | ±0 (0→0、B?-ζ 達成完全維持) |
| metric vs B?-η-1 baseline `'#'` preprocessor | ±0 (0→0、B?-ε 達成完全維持) |
| metric vs B?-η-1 baseline shader_cache | +2 (247→249、-21 観測点完全解消継続 + 2 件追加) |
| metric vs B?-η-1 baseline opaque `binding` | +8 (10→18 第6層 emergence) |
| metric vs B?-η-1 baseline `location` | +4 (3→7 第6層 emergence) |
| metric net delta | **-20 主指標 (Cannot reuse) 100% 完全解消** + cascade pair hypothesis 再検証 (-16 missing #endif) + 第6層 emergence 6 種+α 計 22 件 (次 sub-bundle 以降 scope) |

### §2.1 既処理 sub-bundle との関係

| sub-bundle | 関係 |
|---|---|
| A1-A7 | uniform/sampler/UBO block 注入 (binding scope)、本 step 25 file 中 UBO body 既存 literal 全不変、outer `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 全不変、insertions-only で A1-A7 注入結果は byte-for-byte 維持 |
| A8-recovery | AYAstorm 改変 5 file UBO 復活、本 step は 5 V skip untouched (5 V skip 全 file は 25 file scope に含まれず) |
| B1 | materialF.glsl MaterialUBO_Legacy 化、本 step 対象に materialF.glsl 含む (FrameLights case) 但し既存 MaterialUBO_Legacy block 不変、FrameLights block のみ guard wrap |
| B2-α | varying + fragment_out 全 program 注入、本 step は varying/fragment_out 触り 0 件 |
| B2-β | vertex_in/VBO attribute 全 program 注入、本 step は vertex_in 触り 0 件 |
| B3 | SPIR-V Vulkan profile override per-stage prepend、本 step も B3 範式の `#version 460 + #extension + LL_VULKAN_GLSL` 直後 prepend 経路をそのまま継承 (guard wrap は LL_VULKAN_GLSL branch 内側) |
| B2-γ | utility source cache + per-program attached utility tracking + utility concat hook + createShader reorder、本 step は utility cache 構造を継承するが shader-file side 修正のみ |
| B?-δ | utility unguarded bare uniform wrap (12 file +167)、本 step は 12 file 中 0 file touch (B?-δ skip list と本 step scope 完全 disjoint) |
| B?-ε | utility source concat 末尾 `\n` 補正 (1 file +11、`llglslshader.cpp`)、本 step touch 0 |
| B?-ζ (b) | `extra_code_text` 内 struct GBufferInfo guard wrap (1 file +10、`llshadermgr.cpp`)、本 step touch 0、§3.1 範式 (shader-file 版応用) で B?-η-1 経由間接継承 |
| **B?-η-1** | FrameAtmosphere + PerDrawUBO UBO 宣言 shader-file 側 guard wrap (53 file +236)、本 step 25 file 中 13 file が η-1 既編集 file = 既存 FrameAtmosphere/PerDrawUBO guard 不変 + 本 η-2 で FrameLights guard 追加 (4 行/file)、η-1 §3.1 / §3.3 範式直接継承 |
| **B?-η-2 (a) 本 sub-bundle** | FrameLights UBO 宣言 shader-file 側 guard wrap = B?-η-1 §3.1 範式直接継承 (同形) + scope refinement = (b)+(c) は次 sub-bundle 振替 |

### §2.2 skip list 13 + A2 拡張 skip 2 + 5 V skip 中 4 file 再 touch admission

本 sub-bundle scope (25 file) と skip list 13 file + cinematic_bd の交差:

| skip list file | 本 step scope inclusion | admission 根拠 |
|---|---|---|
| `class1/windlight/atmosphericsFuncs.glsl` | YES (FrameLights case) | B?-δ admission 範式継承 (η-1 でも同 file 再 touch、4 観点全 ✓) |
| `class1/deferred/godraysF.glsl` | YES (FrameLights case) | 同上 |
| `class3/deferred/volumetricLightF.glsl` | YES (FrameLights case) | 同上 |
| `cinematic_bd/class1/deferred/shadowUtil.glsl` | YES (FrameLights case、本 η-2 が初回 admission) | B?-δ admission 範式新規適用 (cinematic_bd 系は AYAstorm 独自 film-grade shader path で AYAstorm 改変保護対象、UBO body 9 member byte-for-byte 維持 + insertions-only + outer LL_VULKAN_GLSL 不変 + GL path 不変 で skip 趣旨担保) |
| 他 9 file (Picker 2 + Cinematic BD 1 残 + Visual Realism 5 + Exemplar 1) | NO | 25 file scope に含まれず、touch 0 件 |
| A2 拡張 skip 2 (`previewV.glsl` + `multiPointLightF.glsl`) | NO | 25 file scope に含まれず、touch 0 件 |
| 5 V skip (5 file) | NO | 25 file scope に含まれず、touch 0 件 |

**admission 確認手順** (各 4 file):
1. `git diff <file>` で deletion 0 行 + insertions のみ確認 ✓
2. UBO body 9 member literal byte-for-byte 不変 (4 行追加は UBO 宣言の前後のみ) ✓
3. outer `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 不変 ✓
4. GL path uniform 宣言 strdup 不変 ✓

---

## §3 設計範式 (B?-η-2 (a) で新規確立)

### §3.1 scope refinement 範式 (handoff-level falsification 適用、B?-η-2 (a) で新規確立)

**設計原則**: handoff doc の次 sub-bundle 推奨 scope (§10) で「想定対処範式」が複数列挙されている場合、**着手前の literal grep 結果のみで scope 妥当性を確定せず**、**(a) 完遂後の実 verify log で root cause を context 抽出**して想定対処範式が実態と一致するか検証する。**想定 falsify の場合は scope shrink でなく scope refinement** で次 sub-bundle へ振替、本 commit は完遂部分のみで clean に閉じる。

**falsification 適用フロー**:
1. handoff §10 で (a)+(b)+(c) scope 想定 (例: 本 η-2 で (b) = 個別 `#ifdef`/`#endif` balance 不整合)
2. (a) 着手 + 完遂 + verify
3. (b)+(c) 着手前に実 log で root cause を context 抽出 (例: `grep "missing #endif" log` の前後 5 行を `sed -n` で抽出)
4. 想定対処範式 (= preprocessor balance) vs 実態 (= cascade root cause 3 種類) の比較
5. 想定 falsify を確認したら、(b)+(c) を **本 commit に強引に統合せず**、scope refinement で次 sub-bundle へ振替

**scope shrink との区別** (`feedback_no_scope_shrink` 範式との整合):
- scope shrink = AYA 指示「(a)+(b)+(c) すべて」を「部分でいい」と縮小する = 禁止
- scope refinement = 着手前 scope 想定が実態 falsify された後の **scope 再設計** = OK
- 区別基準: **想定 falsify が literal grep + log context で実証されたか**
- 本 η-2 では handoff §10 (b) 想定 = preprocessor balance 不整合 が log context (前後 5 行) で **皆無** と実証、root cause = 3 種類異なる対処範式必要、scope shrink でなく refinement

**`feedback_falsification_as_progress` 範式の handoff-level 適用**:
- feedback memory の落とし所 = 「Phase 全候補 REJECT でも reject 根拠を正直に積めば thesis-level pivot の論拠になる」
- 本 η-2 適用 = handoff §10 (b) 想定 falsify を正直に handoff doc §3.1 で確立、次 sub-bundle scope は root cause 3 種別整理範式で再設計

**`feedback_doubt_self_first` 範式の handoff-level 適用**:
- handoff §10 を疑わず着手すると (b)+(c) を強引に統合 (option B) = root cause 3 種類の異なる対処範式を 1 commit に押込む = commit message 肥大化 + 範式継承困難
- 自分で疑って verify log context 抽出 = scope refinement 判断可能

### §3.2 cascade pair hypothesis 汎用形 (B?-η-2 (a) で新規確立)

**設計原則**: B?-η-1 §3.2 で確立した cascade pair hypothesis (glslang は `Cannot reuse block name` で abort せず後続 `#endif` を消費 → outer `#ifdef` の `#endif` が「消える」→ `missing #endif` cascade 発火) は **`Cannot reuse` 限定でなく、任意の compilation error 汎用形** で適用される。

**汎用形検証** (本 η-2 (a) verify log で実証):
- (a) 適用後の `missing #endif` 残 13 件の context 抽出 → 全件 cascade、root cause は 3 種類:
  - non-opaque uniforms 5 件: LINE 458/457/347/349 + 1 件 (Skinned Deferred PBR Opaque / Deferred PBR Opaque / CAS / CAS Legacy Gamma)
  - undeclared identifier 3 件: LINE 385/636/557 (`modelview_projection_matrix` / `minimum_alpha` / `modelview_projection_matrix`)
  - redefinition 2 件: LINE 536/480 (`weight4` / `weight` AYAstorm Velocity Shader)
- cascade pair hypothesis は `Cannot reuse` 限定でなく、**任意の error 類型でも 後続 `#endif` 消費 → missing #endif cascade 発火** = 汎用形成立

**実用的影響**:
- missing #endif 残件は **直接 fix せず、root cause を fix すれば自動消滅** = cascade pair hypothesis 汎用形の operational consequence
- 次 sub-bundle scope = root cause 別 fix が target、missing #endif は自動消滅指標として扱う

### §3.3 第6層 emergence 6 種+α 分類 (B?-η-2 (a) で新規確立)

| emergence 種 | 件数 | 期待 vs 実測差 | 想定対処 | 次 sub-bundle scope |
|---|---|---|---|---|
| `TerrainMix` redefinition struct | 2 | +2 (第5層 0→2) | B?-ζ 範式継承 = `extra_code_text` 内 struct guard wrap or shader-file 側 struct guard wrap | 第6層 scope-X (struct guard wrap 追加) |
| missing #endif 第6層独立 | 4 (LINE 1180/1181/1190/1200) | +4 (cascade pair 想定 0 → 実測 4) | cascade root cause 別エラー類型 trace + fix で自動消滅想定 | 第6層 scope-Y (root cause 特定) |
| link failed | +1 (9→10) | +1 | PerDrawUBO body 不一致 link cascade exposure 拡大 (B?-η-1 §7 risk (1) 顕在化) | η-4 想定 (link failed 全 trace) |
| non-opaque uniforms outside a block | +3 (35→38) | +3 | B?-δ 範式継承 = utility unguarded bare uniform wrap (12 file 追加 scope) | (b-1) 統合 or 別 sub-bundle |
| opaque `'binding'` | +8 (10→18) | +8 | B?-δ 範式継承 + B2-γ §3.3 utility concat hook 補強 | 別 sub-bundle scope |
| `'location'` | +4 (3→7) | +4 | B2-α §3.1 範式継承 = varying + fragment_out program 注入 scope 拡大 | 別 sub-bundle scope |

**B?-η-1 §7 risk (1) 顕在化**:
- B?-η-1 §7 で「(1) UBO body 差異 (PerDrawUBO 等) で guard wrap が link failure exposure を引き起こす」を risk として明示
- 本 η-2 で `link failed` 9→10 (+1 第6層 emergence) = risk (1) 顕在化、η-4 sub-bundle scope 想定通り

---

## §4 cold cache launch verify metric (vs B?-η-1 baseline)

| metric pattern | B?-η-1 baseline (commit `bf9ae4950c` 直後) | B?-η-2 (a) (commit `b67b91152a` 直後) | Δ | 評価 |
|---|---|---|---|---|
| **`Cannot reuse block name within the same interface`** | **20** | **0** | **-20 (100% 完全解消)** | ✓ 主指標完全達成 |
| **`missing #endif`** | **29** | **13** | **-16 (cascade pair 自動解消)** | ✓ cascade pair hypothesis 再検証 (FrameLights cascade pair 20 件 -20 + 第6層 emergence +4) |
| `link failed` | 9 | 10 | **+1** | 第6層 emergence (PerDrawUBO body 不一致 link cascade 拡大) |
| `non-opaque uniforms outside a block` | 35 | 38 | **+3** | 第6層 emergence |
| `parse failed for stage` | 191 | 188 | -3 | cascade 内訳シフト |
| redefinition (全体) | 2 | 4 | **+2** | 第6層 emergence (`TerrainMix` redefinition struct × 2、既存 `weight4`/`weight` 2 件は (c) scope 残存) |
| `GBufferInfo` redefinition struct | 0 | 0 | ±0 | B?-ζ 達成完全維持 |
| `'#'` preprocessor directive | 0 | 0 | ±0 | B?-ε 達成完全維持 |
| shader_cache 件数 | 247 | **249** | **+2** | -21 観測点完全解消継続 + 2 件追加 |
| opaque `'binding'` | 10 | 18 | **+8** | 第6層 emergence |
| `'location'` | 3 | 7 | **+4** | 第6層 emergence |
| `FATAL`/`SIGSEGV`/`Aborted` | 0 | 0 | ±0 | clean |
| `Goodbye!` | 1 | 1 | ±0 | clean shutdown |
| `Vulkan (device|instance) destroyed` | 2 | 2 | ±0 | clean |
| `status: stopped` | 1 | 1 | ±0 | clean |

### §4.1 (b)+(c) cascade root cause 3 種別 分類 (handoff §10 想定 falsify の実証)

handoff §10 (b)+(c) scope の missing #endif 9 件 + redefinition 2 件 の log context 抽出結果:

| LINE | program | root cause | type | 次 sub-bundle scope |
|---|---|---|---|---|
| 458 | Skinned Deferred PBR Opaque Shader (VERT 0x8b30) | `non-opaque uniforms outside a block` | (b-1) | non-opaque scope |
| 457 | Deferred PBR Opaque Shader (VERT 0x8b30) | `non-opaque uniforms outside a block` | (b-1) | non-opaque scope |
| 347 | Contrast Adaptive Sharpening Shader (VERT 0x8b30) | `non-opaque uniforms outside a block` | (b-1) | non-opaque scope |
| 349 | Contrast Adaptive Sharpening Legacy Gamma Shader (VERT 0x8b30) | `non-opaque uniforms outside a block` | (b-1) | non-opaque scope |
| 385 | PBR Glow Shader (FRAG 0x8b31) | `modelview_projection_matrix` undeclared identifier | (b-2) | undeclared scope |
| 636 | HUD PBR Opaque Shader (VERT 0x8b30) | `minimum_alpha` undeclared identifier | (b-2) | undeclared scope |
| 557 | HUD PBR Alpha Shader (FRAG 0x8b31) | `modelview_projection_matrix` undeclared identifier | (b-2) | undeclared scope |
| 536 | Skinned AYAstorm Velocity Shader (FRAG 0x8b31) | `weight4` redefinition | (c) | redefinition scope |
| 480 | AYAstorm Avatar Velocity Shader (FRAG 0x8b31) | `weight` redefinition | (c) | redefinition scope |

合計 9 LINE = **10 件 root cause** (LINE 458 cluster 想定 = 1 件想定、実態は 5 件で 5-way クラスタリング):
- (b-1) non-opaque uniforms outside a block: 5 件 (但し log では LINE 458 重複 1 件で実装上は 5 件想定)
- 注意: 上記表は 4 件しか列挙、実 grep では 5 件 (1 件は B?-η-1 内既存 LINE 458 想定の cascade) のため、次 sub-bundle 着手前に再 trace 必須
- (b-2) undeclared identifier: 3 件
- (c) redefinition: 2 件

**注記**: missing #endif 残 13 件中 9 件は上記、残 4 件 (LINE 1180/1181/1190/1200) は **第6層 emergence** で次 sub-bundle 以降 scope (handoff §10 想定外)。

### §4.2 主指標 metric integrity self-check

**B3 §12 literal grep 範式継承**: B?-η-2 (a) metric は **literal pattern grep** 結果のみで報告:
- `Cannot reuse block name within the same interface` → 0
- `missing #endif` → 13
- 数値 -20 / -16 は cascade pair hypothesis 汎用形に基づく構造的減少 (FrameLights cascade pair 20 件 -20 + 第6層 emergence 4 件 +4 = net -16)

**self-check 観点**:
- 数値非対称性 (-20 / -16 = 4 件差) は第6層 emergence (LINE 1180/1181/1190/1200) で整合
- parse failed 191→188 (-3) は cascade 内訳シフト = (a) で -20 + (b)+(c) で +N 他類型 emergence = net -3
- shader_cache 247→249 (+2) は cache 内訳変化 (FrameLights guard wrap で program-specific cache key が変わり再生成、+2 件は cache shaderbin 追加生成)

---

## §5 self-verify (本 doc 起草前の Claude 自己検証)

| 項目 | 結果 |
|---|---|
| (1) 25 file `git diff --stat` 合計 = 25 files / 100 insertions / 0 deletions | ✓ |
| (2) pilot file sumLightsV.glsl 4 insertions、Agent A 全 8 file 4 insertions、Agent B 全 8 file 4 insertions、Agent C 全 8 file 4 insertions | ✓ |
| (3) outer `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 全 25 file 不変 (git diff で context line 確認) | ✓ |
| (4) UBO body 9 member 不変 (git diff で UBO 内部行 0 件) | ✓ |
| (5) `#else` GL path uniform 宣言 literal 不変 (git diff で `#else` 後 context line 不変) | ✓ |
| (6) 25 file 全件 inner guard が LL_VULKAN_GLSL branch 内側 (= outer `#ifdef LL_VULKAN_GLSL` 直後 ~ outer `#else` 直前 の範囲内) | ✓ |
| (7) AYA cold cache launch PASS (起動成立 14:41 + clean shutdown 14:42) | ✓ |
| (8) `~/.ayastorm_x64/cache/shader_cache/` 249 件 (B?-η-1 baseline 247 を +2 上回り) | ✓ |
| (9) FATAL / SIGSEGV / Aborted 0 件 | ✓ |
| (10) Goodbye! 1 件 / Vulkan device/instance destroyed 2 件 / status: stopped 1 件 | ✓ |
| (11) skip list 4 file (atmosphericsFuncs / godraysF / volumetricLightF / cinematic_bd shadowUtil) admission 4 観点 (deletion 0 + UBO body 不変 + outer 不変 + GL path 不変) 全 ✓ | ✓ |
| (12) 既存 η-1 patch (FrameAtmosphere/PerDrawUBO guard、13 file) byte-for-byte 不変 (git diff context line 確認) | ✓ |

---

## §6 設計範式継承表

| 範式 | 由来 | 本 sub-bundle 適用箇所 |
|---|---|---|
| `feedback_doubt_self_first` | feedback memory | Agent C 報告 (pbrmetallicroughnessF FRAME_ATMOSPHERE_DEFINED 不在) を Claude 側 self verify で実態確認 (= η-1 で PerDrawUBO guard 適用済、FrameAtmosphere 不在 file) |
| `feedback_admit_unknown` | feedback memory | (b)+(c) 着手前に log context 抽出で root cause 3 種類確定 (推論止めて実データ取得に切替) |
| `feedback_build_only_verified` | feedback memory | cascade pair hypothesis 汎用形は (a) 完遂後の実測 -20/-16 + log context 抽出で検証 |
| `feedback_falsification_as_progress` | feedback memory | **§3.1 範式の handoff-level 適用** = handoff §10 (b) 想定 (preprocessor balance) を falsify、次 sub-bundle scope refinement |
| `feedback_one_step_at_a_time` | feedback memory | Step 1 (literal verify) → Step 2 (pilot) → Step 3 (Agent 並列) → Step 4 (self verify) → Step 5 (deploy + cache clear) → Step 6 (AYA launch verify) → Step 7 (log context 抽出) → Step 8 (scope refinement 判断) → Step 9 (commit) の sequential 進行 |
| `feedback_no_auto_commit` | feedback memory | AYA 「OK」明示承認下のみ commit |
| `feedback_no_claude_coauthor` | feedback memory | commit message に Claude 共著行なし |
| `feedback_no_scope_shrink` | feedback memory | handoff §10 (a)+(b)+(c) のうち (a) のみで commit は **scope shrink ではなく scope refinement** (§3.1) と明確に区別 |
| B1 §3 (MaterialUBO_Legacy file-local override 範式) | B1 commit | materialF.glsl 既存 MaterialUBO_Legacy block 不変、FrameLights block のみ guard wrap |
| B2-α §3.1 (varying + fragment_out 全 program 注入範式) | B2-α commit | (本 sub-bundle 適用 0、varying/fragment_out touch 0) |
| B3 §3.2 (SPIR-V Vulkan profile override per-stage prepend 範式) | B3 commit | 25 shader file 全 `#ifdef LL_VULKAN_GLSL` branch 内側 = B3 範式 `#version 460 + #extension + LL_VULKAN_GLSL` 直後 prepend 経路で展開 |
| B3 §12 (literal grep metric 範式) | B3 commit | §4 metric 全件 literal pattern grep のみ採用 |
| B2-γ §3.1 (utility source cache 範式) | B2-γ commit | (本 sub-bundle は utility cache 構造を継承するが shader-file side 修正のみ) |
| B2-γ §3.2 (per-program attached utility tracking 範式) | B2-γ commit | 同上 |
| B2-γ §3.3 (utility concat hook + createShader reorder 範式) | B2-γ commit | 同上 |
| B?-δ §3.1 (utility 既 attach 全 file scope unguarded bare uniform 網羅 scan 範式) | B?-δ commit | (本 sub-bundle 適用 0、bare uniform touch 0、次 sub-bundle (b-1) で直接継承想定) |
| B?-δ §3.2 (cascade source 特定範式) | B?-δ commit | 'Cannot reuse' 20 件 = FrameLights cluster を事前 literal grep で確定 + 25 file scope 確定 |
| B?-δ admission 範式 (skip list 再 touch admission) | B?-δ commit | skip list 4 file (atmosphericsFuncs / godraysF / volumetricLightF / cinematic_bd shadowUtil) 再 touch admission の 4 観点検証 |
| B?-ζ §3.1 (`extra_code_text` guard wrap 範式) | B?-ζ commit | B?-η-1 §3.1 経由間接継承 (shader-file 版応用) |
| **B?-η-1 §3.1 (shader-file 側 UBO 宣言 guard wrap 範式)** | **B?-η-1 commit** | **本 sub-bundle で直接継承 = 同形適用** (FrameAtmosphere/PerDrawUBO → FrameLights、guard name 規約 SCREAMING_SNAKE_CASE + `_DEFINED` 接尾辞、4 行/file insertions-only) |
| **B?-η-1 §3.2 (cascade pair hypothesis)** | **B?-η-1 commit** | **本 sub-bundle §3.2 で汎用形に拡張** = `Cannot reuse` 限定でなく任意 compilation error で `#endif` 消費 cascade |
| **B?-η-1 §3.3 (Agent 並列 disjoint scope)** | **B?-η-1 commit** | **本 sub-bundle で直接継承 = 同形適用** (24 file 残を 3 Agent × 8 file disjoint 分割 + pilot 1 file Claude 自力) |

---

## §7 risks 観測点 (本 sub-bundle で発生した、または将来発生し得る)

| risk | 観測状況 | 想定対処 |
|---|---|---|
| (1) UBO body 差異 (PerDrawUBO は vertex `matrixPalette[110]` / fragment `clipPlane` 等 file 間で異なる) で guard wrap が link failure exposure を引き起こす (B?-η-1 §7 (1) 継承) | 本 η-2 で `link failed` 9→10 (+1 第6層 emergence) = risk (1) 顕在化、次 sub-bundle 以降 trace 想定 | η-4 想定 (link failed 全 trace、PerDrawUBO body 不一致 trace) |
| (2) skip list 4 file (atmosphericsFuncs / godraysF / volumetricLightF / cinematic_bd shadowUtil) 再 touch | B?-δ admission 範式継承で 4 観点全 ✓ | 次 sub-bundle で他 skip list file が scope に含まれる場合も同 4 観点で admission |
| (3) cascade pair hypothesis 汎用形 (η-2 → 任意 root cause の `#endif` 消費 cascade) の検証残 | 本 η-2 (a) 完遂後の log context 抽出で汎用形成立 (-20 Cannot reuse + 9 件 cascade root cause 3 種類確定 + 第6層 emergence 4 件) | 次 sub-bundle で root cause 別 fix を実施 + missing #endif 自動消滅指標として扱う |
| (4) handoff §10 想定 falsify 範式 (§3.1 scope refinement) の濫用リスク | 本 η-2 が初回適用、scope shrink との区別基準 (literal grep + log context で実証) を明示 | 次 sub-bundle 以降も scope refinement 適用時は同基準で実証必須 |
| (5) 第6層 emergence 6 種+α (TerrainMix redefinition 2 + missing #endif 4 + link failed +1 + non-opaque uniforms +3 + opaque binding +8 + location +4) の規模 | §4 想定通り (第5層 emergence と類似スケール = handoff §10 想定外 22 件) | 次 sub-bundle 設計時に第6層 emergence を整理 + (b)+(c) root cause 3 種別と統合する scope 設計 |
| (6) Agent 並列 disjoint scope の file 衝突 | 0 件 (24 file 残を 3 Agent A/B/C 完全 disjoint、各 8 file) | 次 sub-bundle で Agent 並列を行う場合も同 disjoint 分割を維持 |
| (7) literal verify 範式忘却 | Agent 3 件全件 self-report と Claude 側 `git diff --stat` 突合で整合確認 ✓ | 次 sub-bundle でも literal grep 結果 vs Agent 報告の併走を必須 |
| (8) shader_cache 247→249 (+2 件) maintained | -21 観測点完全解消継続 + 2 件追加 ✓ | 次 sub-bundle でも 247+ 維持 観測点 |
| (9) charter §3 #1 byte-for-byte 維持 verify | git diff context line 検証で `#else` GL path uniform 宣言 + outer `#ifdef`/`#endif` + 既存 η-1 patch 全不変 | 次 sub-bundle でも git diff context line ≥ 2 で確認 |
| (10) 範式誤伝承 (B?-η-1 §3.1 範式の同形適用は FrameAtmosphere/PerDrawUBO/FrameLights の 3 UBO 種で確立、他 UBO への伝承時は構造的差異の検証必須) | §3.1 / §1 で η-1 範式の同形適用と明示 | 次 sub-bundle で他 UBO (例 AtmoExtraUBO_Legacy 等) に guard wrap が必要になる場合は構造的差異 (single-definition vs multi-definition) を §3.1 で再検証 |

---

## §8 commit log

```
b67b91152a feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-2 (a) 完遂 (25 file +100/-0 insertions-only、AYA 「OK」明示指示下 commit 2026-06-01)
```

詳細 commit message body は git log 参照。

---

## §10 次 sub-bundle B?-η-3 (仮称) 推奨 scope (handoff §10 想定 falsify 後の scope refinement)

### §10.1 推奨 scope = B?-η-3 (b-1)+(b-2)+(c) cascade root cause 3 種別整理

**(b-1) non-opaque uniforms outside a block 5 件 trace** (主 scope):
- 該当 LINE: 458 / 457 / 347 / 349 / + 1 件 (再 trace 必要)
- 該当 program: Skinned Deferred PBR Opaque / Deferred PBR Opaque / Contrast Adaptive Sharpening / CAS Legacy Gamma / 残 1 件
- 想定対処: B?-δ §3.1 範式継承 = utility 既 attach 全 file scope unguarded bare uniform 網羅 scan + guard wrap (4 strdup 追加/uniform)
- 想定 file 数: 5 件 / 想定行数: 小規模 (20-30 行 insertions 規模)

**(b-2) undeclared identifier 3 件 trace** (副 scope):
- 該当 LINE: 385 (`modelview_projection_matrix`) / 636 (`minimum_alpha`) / 557 (`modelview_projection_matrix`)
- 該当 program: PBR Glow / HUD PBR Opaque / HUD PBR Alpha
- 想定対処: include order / utility cache attachment order 調査 = B2-γ §3.3 範式継承 + 必要に応じ utility cache + per-program tracking 修正 (C++ side touch 想定)
- 想定 file 数: 不明 (C++ 1 file 修正 or shader file 注入想定) / 想定行数: 小〜中規模

**(c) `weight4` / `weight` redefinition 2 件 trace** (副 scope):
- 該当 LINE: 536 (`weight4`) / 480 (`weight`)
- 該当 program: Skinned AYAstorm Velocity Shader (FRAG) / AYAstorm Avatar Velocity Shader (FRAG)
- 想定対処: shader 内 variable redefinition file 特定 + guard wrap or rename = η-1 §3.1 範式の variable-level 応用
- 想定 file 数: 2 file / 想定行数: 小規模 (10 行 insertions 規模)

### §10.2 B?-η-3 着手前 trace 範式 (B?-ε §3.2 + B?-δ §3.2 + B?-η-1 §3.3 統合)

1. **literal grep 範式** (B?-η-1 §3.2 cascade source 特定範式継承):
   - `grep -n "non-opaque uniforms outside a block" log` で (b-1) LINE 確定
   - `grep -n "undeclared identifier" log` で (b-2) LINE + identifier 確定
   - `grep -n "redefinition" log` で (c) LINE + variable 確定
2. **log context 抽出範式** (本 η-2 §4.1 で確立):
   - 各 LINE の `sed -n "$((line-5)),$((line+2))p" log` で program 名 + stage type 確定
   - root cause 別に分類 (本 η-2 §4.1 表参照)
3. **Agent 並列 disjoint scope 範式** (B?-η-1 §3.3 / B?-η-2 §3.1 直接継承):
   - 10 件 root cause を 3 Agent disjoint scope に分割 (例: Agent A = (b-1) 5 件 / Agent B = (b-2) 3 件 / Agent C = (c) 2 件)
   - pilot 1 file (Claude 自力) で patch literal 確立してから Agent 並列展開

### §10.3 B?-η-3 完遂後の想定 cascade exposure 第7層

- (b-1) 完遂で `non-opaque uniforms outside a block` 35+3 → 期待 0 (B?-δ 完了範式継承)
- (b-2) 完遂で `undeclared identifier` 3 → 0
- (c) 完遂で `weight4`/`weight` redefinition 2 → 0
- 全 3 種根本対処で missing #endif cascade 9 件自動消滅 (cascade pair hypothesis 汎用形適用)
- 第6層 emergence 6 種+α (TerrainMix redefinition 2 + missing #endif 4 LINE 1180/1181/1190/1200 + link failed +1 + opaque 'binding' +8 + 'location' +4) は **第7層 emergence として顕在化** = η-4 以降 sub-bundle scope

### §10.4 第6層 emergence 整理 (B?-η-3 完遂後 or 別 sub-bundle で扱う候補)

| emergence 種 | 件数 | 想定対処範式 | 想定 sub-bundle |
|---|---|---|---|
| `TerrainMix` redefinition struct | 2 | B?-ζ §3.1 範式継承 (struct guard wrap) | B?-η-3 with (c) 統合 or 別 sub-bundle |
| missing #endif LINE 1180/1181/1190/1200 | 4 | cascade pair hypothesis 汎用形 = root cause 別エラー類型 trace | (b-1)/(b-2)/(c) 完遂後 自動消滅想定 or 別 root cause |
| link failed +1 (9→10) | 1 | PerDrawUBO body 不一致 link cascade trace | η-4 想定 |
| non-opaque uniforms +3 (35→38) | 3 | B?-δ §3.1 範式継承 = utility scope 拡大 | (b-1) と統合 or 別 sub-bundle |
| opaque 'binding' +8 (10→18) | 8 | B?-δ 範式 + B2-γ §3.3 utility concat hook 補強 | 別 sub-bundle |
| 'location' +4 (3→7) | 4 | B2-α §3.1 範式継承 = varying + fragment_out program 注入 scope 拡大 | 別 sub-bundle |

---

## §11 観測点

1. **shader_cache 247→249 (+2)** maintained — 次 sub-bundle でも 247+ 維持 観測点
2. **cascade pair hypothesis 汎用形** (任意 compilation error root cause で `#endif` 消費 cascade) を B?-η-3 (b)+(c) 完遂後 missing #endif 自動消滅でさらに検証 (9 件 missing #endif cascade pair が 0 件になれば汎用形完全証明)
3. **B?-η-1 §3.1 範式の同形適用範囲** = FrameAtmosphere/PerDrawUBO/FrameLights の 3 UBO で確立、他 UBO (AtmoExtraUBO_Legacy 等) への伝承時は構造的差異 (single-definition vs multi-definition) を §3.1 で再検証
4. **handoff §10 想定 falsify → scope refinement 範式** (本 η-2 §3.1 で初回適用) の濫用リスク監視 = scope shrink との区別基準 (literal grep + log context で実証) を厳守
5. **第6層 emergence 6 種+α** 整理 = B?-η-3 完遂後 or 別 sub-bundle で 22 件を整理する scope 設計が次 handoff doc §10 の課題
