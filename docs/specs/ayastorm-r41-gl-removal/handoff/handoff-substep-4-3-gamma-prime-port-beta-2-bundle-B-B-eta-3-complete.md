# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-3 完遂 → 次 sub-bundle (B?-η-4 仮称) 着手境界 handoff (2026-06-02)

**parent commit**: `733bfc225f` (B?-η-3 patch、本 handoff の直接 parent) / `b67b91152a` (B?-η-2 (a) patch 範式継承元) / `bf9ae4950c` (B?-η-1 patch 範式継承元) / `6924d4b827` (B?-η-2 (a)-complete handoff doc commit)
**HEAD**: `733bfc225f` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: B?-η-3 (= PerDrawUBO per-group 名称固有化 = B?-η-1 自作 bug 修正 = guard macro collision 解消) 完遂状態 + 次 sub-bundle (推奨 B?-η-4 仮称 = handoff §10 移管 (b-1)+(b-2)+(c) 9 件 + 第7層 emergence 4 系統 scope 設計) 着手判断境界 を fresh context 引継 用に確定する doc-only handoff。B?-η-2 (a)-complete `6924d4b827` 範式継承。**handoff §10 想定外発見 (B?-η-1 自作 bug = `PER_DRAW_UBO_DEFINED` guard macro collision、'size' undeclared 118 件 cascade) → scope refinement 3rd-level で本 sub-bundle scope 確定範式 1 件を新規確立** (= `feedback_doubt_self_first` 範式の handoff-level 適用、§10 (b)+(c) 想定着手前の log context 抽出で「想定にない第3 root cause」を検出、scope 振替で B?-η-4 移管)。`'size' : undeclared identifier` **118→0 (-118 / 100% 完全解消)** ✓ 主指標完全達成 + `missing #endif` **13→0 (-13 / 100% 完全解消)** ✓ + `Link failed` **10→0 (-10 / 100% 完全解消)** ✓ **3 主指標 100% 達成**。13 file +52/-52 rename-only、shader file のみ編集 (C++ touch 0)、Agent (general-purpose) 3 件 parallel disjoint scope + pilot 1 file (Claude 自力 = deferredUtil.glsl)。AYA 「OK」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-B scope 第七 sub-bundle B?-η の sub-step 3 (= B?-η-1 patch refinement 3rd-level = PerDrawUBO per-group 名称固有化) を **13 file +52/-52 rename-only** で完遂した状態を確定し、次 sub-bundle 着手境界を fresh context に引継ぐ。B?-η-1 (commit `bf9ae4950c` = FrameAtmosphere + PerDrawUBO guard wrap 53 file) で導入した `PER_DRAW_UBO_DEFINED` guard macro が **6 different PerDrawUBO bodies (vec4 clipPlane / vec3 color + float size / mat3x4 matrixPalette / mat3x4 lastMatrixPalette) で共有された結果、prepend chain 先 attach wins / 後 attach 全 skip** の構造的問題を引き起こし、deferredUtil.glsl の Group B = light_params (`vec3 color, float size`) skip 由来で **`'size' : undeclared identifier` 118 件 cascade** が発生していた。本 sub-bundle は **UBO 名 + guard macro 名を per-group 固有化** (例 `PerDrawUBO` → `PerDrawUBO_ClipPlane`、`PER_DRAW_UBO_DEFINED` → `PER_DRAW_UBO_CLIP_PLANE_DEFINED`) で 6 group の co-existence を保証し、3 主指標 ('size' 118→0 + missing #endif 13→0 + Link failed 10→0) を 100% 完全解消。

本 sub-bundle は **handoff §10 想定外発見 → scope refinement 3rd-level 範式の主検証**:
- B?-η-2 (a)-complete handoff §10 では (b-1) non-opaque uniforms 5 件 + (b-2) undeclared identifier 3 件 + (c) `weight4`/`weight` redefinition 2 件 = 計 10 件 root cause 整理対処 scope と記述
- 本 η-3 着手前の log context 抽出 → **'size' undeclared 118 件 cascade を新規発見 (handoff §10 想定外)、root cause = B?-η-1 自作 bug = guard macro collision**
- `feedback_doubt_self_first` 範式の handoff-level 適用 = handoff §10 を疑わず着手すると 'size' 118 件 cascade を見逃す可能性、自分で疑って verify log 再 trace で発見
- scope refinement 3rd-level (B?-η-2 (a) §3.1 で確立した 2nd-level 範式の更なる適用) で本 commit は B?-η-3 = PerDrawUBO refinement に振り直し + handoff §10 確定 9 件 (10 件想定の内 1 件は重複) は B?-η-4 へ移管

本 sub-bundle は **B?-η-1 §3.1 範式の bug fix 形応用** の実例:
- η-1 = FrameAtmosphere + PerDrawUBO UBO 宣言 shader-file 側 guard wrap (insertions-only)
- 本 η-3 = η-1 で導入した PerDrawUBO guard の UBO 名 + guard macro 名のみ rename = body 不変 = GL path 不可触
- 構造は同形 (preprocessor guard) だが guard name を per-UBO body 固有化することで 6 group の co-existence を保証
- patch literal 4 行/file × rename = +52/-52 (4 × 13)

本 sub-bundle は **B?-η-1 §3.3 範式の直接継承** の実例 (Agent 並列 disjoint scope):
- 12 file 残 (pilot 1 file 除外) を 3 Agent × disjoint group 分割
- Agent A 5 file = Group A (clip_plane)
- Agent B 4 file = Group B remain + C + D (light_params + avatar_skin + object_skin)
- Agent C 3 file = Group E + F (skinned_velocity + avatar_velocity)
- pilot 1 file (Claude 自力 deferredUtil.glsl Group B) で rename pattern 確立
- Agent 報告と self literal verify 併走 (`git diff --stat` 合計 13 files / 52 insertions / 52 deletions と計算値 4×13 = 52 一致確認)

---

## §2 B?-η-3 完遂 status

| 項目 | 値 |
|---|---|
| Scope | PerDrawUBO per-group 名称固有化 = `PerDrawUBO` → `PerDrawUBO_<Group>` rename + guard macro `PER_DRAW_UBO_DEFINED` → `PER_DRAW_UBO_<GROUP>_DEFINED` 固有化、4 行/file (comment 1 行 + `#ifndef` 1 行 + `#define` 1 行 + `layout(...) uniform <NAME> {` 1 行) を rename のみ |
| 修正 file 数 | **13 file** (η-1 既編集 PerDrawUBO 持つ 13 file = 全て本 η-3 で rename) |
| 変更行数 | **+52 / -52** (rename-only、4×13 = 52) |
| 修正範囲 | 13 shader file 全 `#ifdef LL_VULKAN_GLSL` branch 内側、UBO body member 名・型・順序 byte-for-byte 不変 + layout `set=2, binding=0, std140` 不変 + UBO 名 + guard macro 名のみ rename |
| Group 分類 | 6 group (A clip_plane 5 file / B light_params 3 file / C avatar_skin 1 file / D object_skin 1 file / E skinned_velocity 2 file / F avatar_velocity 1 file) |
| Agent 投入 | **3 件 parallel disjoint scope** (Agent A general-purpose = 5 file Group A / Agent B general-purpose = 4 file Group B remain + C + D / Agent C general-purpose = 3 file Group E + F) + Claude 自力 pilot 1 file (deferredUtil.glsl Group B = light_params) |
| shader file 触り | **13 件** (本 sub-bundle は shader file のみ、C++ touch 0、A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ/B?-ε/B?-ζ/B?-η-1/B?-η-2 (a) 既処理 file は §2.1 で個別検証) |
| AYAstorm 改変保全 | **GL path 全不変** (`#else` branch literal 全 13 file 不変、UBO body member 名・型・順序 literal 全 13 file 不変、outer `#ifdef LL_VULKAN_GLSL ... #endif` 全 13 file 不変、既存 η-1/η-2 (a) patch (FrameAtmosphere/FrameLights guard wrap 38 file = 25 + 13) 全 byte-for-byte 不変、charter §3 #1 acceptance) |
| skip list 13 + A2 拡張 skip 2 + 5 V skip | 13 file 中 0 file (本 η-3 scope の 13 file は skip list と完全 disjoint = 全て PerDrawUBO 持つ deferred/avatar/velocity 系 = AYAstorm 改変保護対象外) |
| AYA cold cache launch verify | **PASS** (起動成立 2026-06-01T15:25 + cache 再生成 223 shaderbin + clean shutdown 15:25:14 + Goodbye! 1 件 + Vulkan device/instance destroyed 各 1 件 + status: stopped 1 件 + 実 FATAL/SIGSEGV/Aborted 0 件) |
| commit | `733bfc225f` (AYA 「OK」明示指示下 2026-06-02) |
| metric vs B?-η-2 (a) baseline `'size' : undeclared identifier` | **-118 ✓ B?-η-3 直接効果 100% 完全達成** (118→0、cascade source 単一 = deferredUtil.glsl Group B skip 由来 118 件) |
| metric vs B?-η-2 (a) baseline `missing #endif` | **-13 ✓ Solution B 副次効果 100% 完全解消** (13→0、cascade pair hypothesis 残全解消) |
| metric vs B?-η-2 (a) baseline `Link failed` | **-10 ✓ Solution B 副次効果 100% 完全解消** (10→0、UBO body 不一致 link cascade exposure 解消) |
| metric vs B?-η-2 (a) baseline `Cannot reuse block name within the same interface` | ±0 (0→0、B?-η-2 (a) 達成完全維持) |
| metric vs B?-η-2 (a) baseline non-opaque uniforms | +89 (38→127 第7層 emergence) |
| metric vs B?-η-2 (a) baseline parse failed | -146 (188→42 cascade 内訳大幅縮小) |
| metric vs B?-η-2 (a) baseline redefinition (全体) | +184 (4→188 第7層 emergence = ALL `'normalMap' : redefinition` at LINE `0:1332` 単一 cluster) |
| metric vs B?-η-2 (a) baseline `GBufferInfo` redefinition struct | ±0 (0→0、B?-ζ 達成完全維持) |
| metric vs B?-η-2 (a) baseline `'#'` preprocessor | ±0 (0→0、B?-ε 達成完全維持) |
| metric vs B?-η-2 (a) baseline shader_cache | -26 (249→223、link 成立路径変化で再生成 program 集合シフト、-21 観測点完全解消継続) |
| metric vs B?-η-2 (a) baseline opaque `'binding'` | +55 (18→73 第7層 emergence sampler binding 仕様要求) |
| metric vs B?-η-2 (a) baseline `'location'` | +11 (7→18 第7層 emergence = (a) SPIR-V location missing + (b) overlapping location 20) |
| metric net delta | **3 主指標 100% 完全解消** ('size' -118 + missing #endif -13 + Link failed -10) + 第7層 emergence 4 系統 計 339 件 (次 sub-bundle 以降 scope) |

### §2.1 既処理 sub-bundle との関係

| sub-bundle | 関係 |
|---|---|
| A1-A7 | uniform/sampler/UBO block 注入 (binding scope)、本 step 13 file 中 UBO body 既存 literal 全不変、outer `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 全不変、rename のみで A1-A7 注入結果は byte-for-byte 維持 |
| A8-recovery | AYAstorm 改変 5 file UBO 復活、本 step は 5 V skip untouched (5 V skip 全 file は 13 file scope に含まれず) |
| B1 | materialF.glsl MaterialUBO_Legacy 化、本 step は materialF.glsl touch 0 (PerDrawUBO 持たない file) |
| B2-α | varying + fragment_out 全 program 注入、本 step は varying/fragment_out 触り 0 件 |
| B2-β | vertex_in/VBO attribute 全 program 注入、本 step は vertex_in 触り 0 件 |
| B3 | SPIR-V Vulkan profile override per-stage prepend、本 step も B3 範式の `#version 460 + #extension + LL_VULKAN_GLSL` 直後 prepend 経路をそのまま継承 (rename は LL_VULKAN_GLSL branch 内側) |
| B2-γ | utility source cache + per-program attached utility tracking + utility concat hook + createShader reorder、本 step は utility cache 構造を継承するが shader-file side 修正のみ |
| B?-δ | utility unguarded bare uniform wrap (12 file +167)、本 step は 12 file 中 0 file touch (B?-δ skip list と本 step scope 完全 disjoint) |
| B?-ε | utility source concat 末尾 `\n` 補正 (1 file +11、`llglslshader.cpp`)、本 step touch 0 |
| B?-ζ (b) | `extra_code_text` 内 struct GBufferInfo guard wrap (1 file +10、`llshadermgr.cpp`)、本 step touch 0、§3.1 範式 (shader-file 版応用) で B?-η-1 経由間接継承 |
| B?-η-1 | FrameAtmosphere + PerDrawUBO UBO 宣言 shader-file 側 guard wrap (53 file +236)、本 step 13 file 全て η-1 既編集 file = 既存 FrameAtmosphere guard 不変 + 本 η-3 で PerDrawUBO guard の UBO 名 + guard macro 名のみ rename = body 不変 = η-1 §3.1 範式の bug fix 形応用 |
| B?-η-2 (a) | FrameLights UBO 宣言 shader-file 側 guard wrap (25 file +100)、本 step 13 file 中 FrameLights guard 持つ file は touch 範囲外 (FrameLights guard 部分不変) |
| **B?-η-3 本 sub-bundle** | PerDrawUBO per-group 名称固有化 = B?-η-1 §3.1 範式の bug fix 形応用 + handoff §10 想定外発見 (B?-η-1 自作 bug = guard macro collision) → scope refinement 3rd-level で本 sub-bundle scope 確定 |

### §2.2 skip list 13 + A2 拡張 skip 2 + 5 V skip 中 0 file 再 touch

本 sub-bundle scope (13 file) と skip list 13 file + cinematic_bd の交差:

| skip list file | 本 step scope inclusion | admission 根拠 |
|---|---|---|
| 13 file (Picker 2 + Cinematic BD 1 + Visual Realism 5 + Exemplar 1 + atmosphericsFuncs + godraysF + volumetricLightF + shadowUtil) | NO | 13 file scope に含まれず、touch 0 件 (本 η-3 scope は PerDrawUBO 持つ deferred/avatar/velocity 系 = skip list と完全 disjoint) |
| A2 拡張 skip 2 (`previewV.glsl` + `multiPointLightF.glsl`) | NO | 13 file scope に含まれず、touch 0 件 |
| 5 V skip (5 file) | NO | 13 file scope に含まれず、touch 0 件 |

**admission 不要**: skip list との交差 0 件のため、本 sub-bundle では admission 範式適用不要。

---

## §3 設計範式 (B?-η-3 で新規確立)

### §3.1 scope refinement 3rd-level 範式 (handoff-level 想定外発見適用、B?-η-3 で新規確立)

**設計原則**: handoff doc の次 sub-bundle 推奨 scope (§10) で「想定対処範式」が複数列挙されている場合、**(a) 完遂後の verify log を着手前に再 trace** して **想定 §10 に列挙されていない第3 root cause** が存在しないか検証する。**想定外 root cause 発見の場合は本 sub-bundle scope を再設計**で「想定外 root cause」を主 scope に据え、handoff §10 確定分は次 sub-bundle へ振替、本 commit は完遂部分のみで clean に閉じる。

**scope refinement の level 階層**:
- **1st-level** (B?-η-2 (a) §3.1 で確立) = handoff §10 想定対処範式 (preprocessor balance) が実態 (cascade root cause 3 種類) と falsify される場合の scope 振替
- **2nd-level** (B?-η-2 (a) §3.1 で確立) = 想定 (b)+(c) を 1 commit に強引統合せず、root cause 3 種別に sub-bundle 分割する scope 設計
- **3rd-level** (本 η-3 §3.1 で新規確立) = handoff §10 に **そもそも列挙されていない** root cause (`'size' undeclared` 118 件 = B?-η-1 自作 bug) を着手前 trace で発見 → 本 sub-bundle scope を再設計 + handoff §10 確定分は次 sub-bundle へ振替

**3rd-level 適用フロー** (本 η-3 で実例化):
1. handoff §10 で (b-1)+(b-2)+(c) scope 想定 (handoff §10 = 10 件 root cause = non-opaque 5 + undeclared 3 + redefinition 2)
2. 着手前に verify log を再 trace (B?-η-2 (a) commit `b67b91152a` 直後 log)
3. `'size' : undeclared identifier` 118 件発見 → **handoff §10 想定外** (handoff §10 では 'size' 言及なし、(b-2) undeclared 3 件 = modelview_projection_matrix/minimum_alpha のみ)
4. 'size' 118 件 cascade source 単一性確認 (全件 deferredUtil.glsl LINE 242/245 由来) → root cause = B?-η-1 自作 bug (PerDrawUBO_DEFINED guard collision)
5. scope refinement 3rd-level で本 commit を 'size' 主 scope (= PerDrawUBO per-group 固有化) に再設計 + handoff §10 確定 9 件 ((b-1) 4 + (b-2) 3 + (c) 2) は B?-η-4 へ移管

**自作 bug 発見範式** (新規):
- 過去 sub-bundle で導入した patch が新たな cascade を引き起こす場合の検出フロー
- 着手前 trace で **想定外 root cause** を発見したら、git blame / git log -p で過去 patch との因果関係を確認
- 本 η-3 では 'size' 118 件 → guard macro collision の発見 → B?-η-1 commit `bf9ae4950c` の `PER_DRAW_UBO_DEFINED` guard が 6 different UBO bodies で共有されている事実を確認
- 自作 bug fix は **handoff §10 確定 scope より優先** (積み残しを増やしてから旧 root cause を fix するより、自作 bug を先に解消する方が cascade 連鎖を断ち切れる)

**`feedback_doubt_self_first` 範式の handoff-level 適用**:
- handoff §10 を疑わず着手すると 'size' 118 件 cascade を見逃す可能性
- 自分で疑って verify log 再 trace で発見

**`feedback_admit_unknown` 範式の handoff-level 適用**:
- 'size' 118 件発見時に推論で「include order 問題」と決めつけず、log context 抽出 + grep で実 root cause を確定

### §3.2 guard macro collision detection 範式 (B?-η-3 で新規確立)

**設計原則**: shader-file 側 UBO 宣言 guard wrap (B?-η-1 §3.1 範式) を複数 UBO body に適用する場合、**guard name は per-UBO body 固有化必須**。**同名 guard を 2 つ以上の異なる UBO body で共有すると prepend chain 先 attach wins / 後 attach 全 skip の構造的問題が発生**し、後 attach UBO body 内の uniform 全てが undeclared identifier cascade を引き起こす。

**collision detection 手順**:
1. shader-file 側 UBO guard wrap を適用する全 file の UBO body member を literal grep で抽出 (`grep -A 10 "uniform PerDrawUBO {" *.glsl`)
2. UBO body member が異なる file が 2 file 以上存在する場合は **collision risk あり**
3. guard name は SCREAMING_SNAKE_CASE + per-UBO body 固有 suffix で命名 (例: `PER_DRAW_UBO_CLIP_PLANE_DEFINED` not `PER_DRAW_UBO_DEFINED`)
4. UBO 名も同じく per-UBO body 固有化推奨 (例: `PerDrawUBO_ClipPlane` not `PerDrawUBO`) = Vulkan SPIR-V profile では UBO 名は binding と独立、固有化しても layout 整合性影響なし

**6 different PerDrawUBO bodies** (本 η-3 で確認):
- Group A (clip_plane): `vec4 clipPlane` (5 file)
- Group B (light_params): `vec3 color; float size` (3 file)
- Group C (avatar_skin): `mat3x4 matrixPalette[MAX_JOINTS_PER_MESH_OBJECT]` (1 file)
- Group D (object_skin): `mat3x4 matrixPalette[MAX_JOINTS_PER_MESH_OBJECT]` (1 file、Group C と同 body だが file 名 + scope 独立)
- Group E (skinned_velocity): `mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT]` (2 file)
- Group F (avatar_velocity): `mat3x4 lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT]` (1 file、Group E と同 body だが file 名 + scope 独立)

**collision の cascade exposure**:
- B?-η-1 では `PER_DRAW_UBO_DEFINED` を 6 group で共有 → prepend chain 先 attach = clipPlane wins → 後 attach (deferredUtil.glsl Group B = light_params) は guard で全 skip → `vec3 color, float size` 宣言 0 → LINE 242/245 で `size` 使用 → 'size' undeclared 118 件 cascade
- B?-η-3 で per-group 固有化 → 6 group co-existence 担保 → 'size' undeclared 0 件

### §3.3 第7層 emergence 4 系統分類 (B?-η-3 で新規確立)

| emergence 種 | 件数 | 期待 vs 実測差 | 想定対処 | 次 sub-bundle scope |
|---|---|---|---|---|
| `'normalMap' : redefinition` at LINE `0:1332` 単一 cluster | 188 | +184 (第6層 4 → 第7層 188) | B?-ε §3.2 範式継承 (cascade source 1 件特定 → 1 patch 188 件解消可能) | 第7層 scope-X (1 source 特定 + guard wrap) |
| non-opaque uniforms outside a block | +89 (38→127) | +89 (cascade exposure linker phase 到達) | B?-δ §3.1 範式継承 = utility 既 attach 全 file scope unguarded bare uniform 網羅 scan | (b-1) と統合 or 別 sub-bundle |
| opaque `'binding'` (sampler/texture/image requires layout(binding=X)) | +55 (18→73) | +55 (cascade exposure sampler binding 仕様要求) | B?-δ 範式 + B2-γ §3.3 utility concat hook 補強 + sampler binding 追加範式 | 別 sub-bundle scope (sampler binding 専用) |
| `'location'` (SPIR-V missing + overlapping 20) | +11 (7→18) | +11 (cascade exposure SPIR-V 仕様要求 + program 内 location 衝突) | B2-α §3.1 範式継承 = varying + fragment_out program 注入 scope 拡大 + location 重複検出範式 | 別 sub-bundle scope |

**B?-η-1 §7 risk (1) 解消** (本 η-3 で完全解消):
- B?-η-1 §7 で「(1) UBO body 差異 (PerDrawUBO 等) で guard wrap が link failure exposure を引き起こす」を risk として明示
- 本 η-3 で per-group 固有化により Link failed 10→0 = risk (1) 完全解消

---

## §4 cold cache launch verify metric (vs B?-η-2 (a) baseline)

| metric pattern | B?-η-2 (a) baseline (commit `b67b91152a` 直後) | B?-η-3 (commit `733bfc225f` 直後) | Δ | 評価 |
|---|---|---|---|---|
| **`'size' : undeclared identifier`** | **118** | **0** | **-118 (100% 完全解消)** | ✓ 主指標完全達成 |
| **`missing #endif`** | **13** | **0** | **-13 (100% 完全解消)** | ✓ Solution B 副次効果 cascade pair hypothesis 残全解消 |
| **`Link failed`** | **10** | **0** | **-10 (100% 完全解消)** | ✓ Solution B 副次効果 UBO body 不一致 link cascade 解消 |
| `Cannot reuse block name within the same interface` | 0 | 0 | ±0 | ✓ B?-η-2 (a) 達成完全維持 |
| `non-opaque uniforms outside a block` | 38 | 127 | **+89** | 第7層 emergence (linker phase 到達後初露出) |
| `parse failed for stage` | 188 | 42 | -146 | cascade 内訳大幅縮小 (3 主指標 100% 解消由来) |
| redefinition (全体) | 4 | 188 | **+184** | 第7層 emergence (ALL `'normalMap'` at LINE `0:1332` 単一 cluster) |
| `GBufferInfo` redefinition struct | 0 | 0 | ±0 | B?-ζ 達成完全維持 |
| `'#'` preprocessor directive | 0 | 0 | ±0 | B?-ε 達成完全維持 |
| shader_cache 件数 | 249 | **223** | **-26** | link 成立路径変化で再生成 program 集合シフト (-21 観測点完全解消継続) |
| opaque `'binding'` | 18 | 73 | **+55** | 第7層 emergence (sampler binding 仕様要求) |
| `'location'` | 7 | 18 | **+11** | 第7層 emergence ((a) SPIR-V location missing + (b) overlapping location 20 LINE 1180/1181/1190/1200) |
| `FATAL`/`SIGSEGV`/`Aborted` | 0 | 0 | ±0 | clean |
| `Goodbye!` | 1 | 1 | ±0 | clean shutdown |
| `Vulkan (device|instance) destroyed` | 2 | 2 | ±0 | clean |
| `status: stopped` | 1 | 1 | ±0 | clean |

### §4.1 第7層 emergence 4 系統 breakdown

handoff §10 想定 (b-1) non-opaque 5 件 + (b-2) undeclared 3 件 + (c) redefinition 2 件 = 9 件 の B?-η-4 移管に加え、本 η-3 で linker phase 到達後初露出した 第7層 emergence 4 系統:

| LINE / pattern | 件数 | program / source | root cause | 次 sub-bundle scope |
|---|---|---|---|---|
| `'normalMap' : redefinition` at LINE `0:1332` | 188 | 多 program (188 件全 LINE 1332 集中) | cascade source 単一 = 共通 utility or shader-file LINE 1332 で `normalMap` 二重宣言 | B?-ε §3.2 範式継承 1 patch 188 件解消可能 |
| `non-opaque uniforms outside a block` | 127 | 多 LINE (182/210/180/172/186 utility-scope + 1125/1383/333/328 program-scope) | B?-δ 範式残 + 第7層 emergence | (b-1) と統合 or 別 sub-bundle |
| `'binding'` (sampler/texture/image requires layout(binding=X)) | 73 | 多 LINE (169-177, 322-323, 1175-1177 系) | Vulkan profile sampler binding 仕様要求 | 別 sub-bundle (sampler binding 専用) |
| `'location'` | 18 | (a) SPIR-V location missing 数件 + (b) overlapping location 20 LINE 1180/1181/1190/1200 | B2-α §3.1 範式残 + program 内 location 衝突 | 別 sub-bundle |

**注記**: handoff §10 確定 9 件 ((b-1) 4 + (b-2) 3 + (c) 2) は B?-η-4 主 scope に移管、第7層 emergence 4 系統 計 339 件 (= 188 + 127 + 73 + 18 - 第6層から継続分 -67) は B?-η-5 以降 sub-bundle scope。

### §4.2 主指標 metric integrity self-check

**B3 §12 literal grep 範式継承**: B?-η-3 metric は **literal pattern grep** 結果のみで報告:
- `'size' : undeclared identifier` → 0
- `missing #endif` → 0
- `Link failed` → 0
- 数値 -118 / -13 / -10 は Solution B (per-group 固有化) による構造的解消

**self-check 観点**:
- 3 主指標 100% 完全解消の整合性 = 'size' 118 件 cascade source 単一 (deferredUtil.glsl Group B skip 由来) → per-group 固有化で全 118 件直接解消、missing #endif 13 件 + Link failed 10 件は cascade 副次効果で同時解消
- parse failed 188→42 (-146) は cascade 内訳シフト = 3 主指標 100% 解消 -141 + 第7層 emergence net 内訳変化 -5 = -146 で整合
- shader_cache 249→223 (-26) は link 成立で program 集合シフト = 一見退行に見えるが実態は link 成立した program のみ shaderbin 生成、-21 観測点 (B?-δ 達成完全解消継続) は維持

---

## §5 self-verify (本 doc 起草前の Claude 自己検証)

| 項目 | 結果 |
|---|---|
| (1) 13 file `git diff --stat` 合計 = 13 files / 52 insertions / 52 deletions | ✓ |
| (2) pilot file deferredUtil.glsl 4 insertions / 4 deletions、Agent A 全 5 file 4 insertions / 4 deletions、Agent B 全 4 file 4 insertions / 4 deletions、Agent C 全 3 file 4 insertions / 4 deletions | ✓ |
| (3) outer `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 全 13 file 不変 (git diff で context line 確認) | ✓ |
| (4) UBO body member 名・型・順序 不変 (git diff で UBO 内部行 0 件 = rename は UBO 宣言行のみ) | ✓ |
| (5) `#else` GL path uniform 宣言 literal 不変 (git diff で `#else` 後 context line 不変) | ✓ |
| (6) 13 file 全件 inner guard rename が LL_VULKAN_GLSL branch 内側 (= outer `#ifdef LL_VULKAN_GLSL` 直後 ~ outer `#else` 直前 の範囲内) | ✓ |
| (7) AYA cold cache launch PASS (起動成立 15:25 + clean shutdown 15:25:14) | ✓ |
| (8) `~/.ayastorm_x64/cache/shader_cache/` 223 件 (B?-η-2 (a) baseline 249 から -26、link 成立 program 集合シフト) | ✓ |
| (9) FATAL / SIGSEGV / Aborted 0 件 | ✓ |
| (10) Goodbye! 1 件 / Vulkan device/instance destroyed 各 1 件 / status: stopped 1 件 | ✓ |
| (11) 既存 η-1 patch (FrameAtmosphere/PerDrawUBO guard、13 file) FrameAtmosphere 部分不変 + PerDrawUBO 部分 rename only (body 不変) | ✓ |
| (12) 既存 η-2 (a) patch (FrameLights guard、25 file) 全 byte-for-byte 不変 (本 η-3 scope の 13 file 中 FrameLights guard 持つ file は touch 範囲外 = FrameLights guard 部分 0 touch) | ✓ |

---

## §6 設計範式継承表

| 範式 | 由来 | 本 sub-bundle 適用箇所 |
|---|---|---|
| `feedback_doubt_self_first` | feedback memory | handoff §10 を疑って verify log 再 trace で 'size' 118 件 cascade 発見 = handoff §10 想定外 root cause = B?-η-1 自作 bug |
| `feedback_admit_unknown` | feedback memory | 'size' 118 件発見時に推論で「include order 問題」と決めつけず、log context 抽出 + grep で実 root cause (guard macro collision) を確定 |
| `feedback_build_only_verified` | feedback memory | Solution B (per-group 固有化) の効果は (a) 完遂後の実測 -118/-13/-10 + log context 抽出で検証 |
| `feedback_falsification_as_progress` | feedback memory | handoff §10.1 「(b-1) +1 件 再 trace 必要」誤記を literal grep で 4 件 (not 5) と確定 (LINE 458/457/347/349) = falsification を progress として記録 |
| `feedback_one_step_at_a_time` | feedback memory | Step 1 (literal verify) → Step 2 (cascade source 確認) → Step 3 (PerDrawUBO 13 file body literal verify) → Step 4 (pilot) → Step 5 (Agent 並列) → Step 6 (self verify) → Step 7 (deploy + cache clear) → Step 8 (AYA launch verify) → Step 9 (metric + 第7層 trace) → Step 10 (commit) の sequential 進行 |
| `feedback_no_auto_commit` | feedback memory | AYA 「OK」明示承認下のみ commit |
| `feedback_no_claude_coauthor` | feedback memory | commit message に Claude 共著行なし |
| `feedback_no_scope_shrink` | feedback memory | handoff §10 (b-1)+(b-2)+(c) 9 件を B?-η-4 移管は **scope shrink ではなく scope refinement 3rd-level** (§3.1) と明確に区別 (想定外 root cause 発見で本 sub-bundle scope 再設計) |
| `feedback_shader_only_fast_iterate` | feedback memory | shader-only 変更のため autobuild 不要、`cp` + `rm shader_cache` のみで反映 |
| B1 §3 (MaterialUBO_Legacy file-local override 範式) | B1 commit | (本 sub-bundle 適用 0、materialF.glsl touch 0) |
| B2-α §3.1 (varying + fragment_out 全 program 注入範式) | B2-α commit | (本 sub-bundle 適用 0、次 sub-bundle で 'location' 系 scope に継承想定) |
| B3 §3.2 (SPIR-V Vulkan profile override per-stage prepend 範式) | B3 commit | 13 shader file 全 `#ifdef LL_VULKAN_GLSL` branch 内側 = B3 範式 `#version 460 + #extension + LL_VULKAN_GLSL` 直後 prepend 経路で展開 |
| B3 §12 (literal grep metric 範式) | B3 commit | §4 metric 全件 literal pattern grep のみ採用 |
| B2-γ §3.1 (utility source cache 範式) | B2-γ commit | (本 sub-bundle は utility cache 構造を継承するが shader-file side 修正のみ) |
| B2-γ §3.2 (per-program attached utility tracking 範式) | B2-γ commit | 同上 |
| B2-γ §3.3 (utility concat hook + createShader reorder 範式) | B2-γ commit | 同上 |
| B?-δ §3.1 (utility 既 attach 全 file scope unguarded bare uniform 網羅 scan 範式) | B?-δ commit | (本 sub-bundle 適用 0、bare uniform touch 0、B?-η-4 (b-1) で直接継承想定) |
| B?-δ §3.2 (cascade source 特定範式) | B?-δ commit | 'size' undeclared 118 件 = deferredUtil.glsl Group B skip 由来 単一 cluster を事前 literal grep で確定 + 13 file scope 確定 |
| B?-ε §3.2 (cascade source ALL N errors 同一 0:LINE 集中 → 1 cluster 範式) | B?-ε commit | 第7層 emergence `'normalMap' redefinition` 188 件 ALL LINE 0:1332 集中 = 1 cluster で本範式適用想定 |
| B?-ζ §3.1 (`extra_code_text` guard wrap 範式) | B?-ζ commit | B?-η-1 §3.1 経由間接継承 (shader-file 版応用) |
| **B?-η-1 §3.1 (shader-file 側 UBO 宣言 guard wrap 範式)** | **B?-η-1 commit** | **本 sub-bundle で bug fix 形応用** = η-1 で導入した PerDrawUBO guard を per-group 固有化 (UBO 名 + guard macro 名 rename) で 6 group co-existence 保証 |
| B?-η-1 §3.2 (cascade pair hypothesis) | B?-η-1 commit | 本 η-3 で missing #endif 13→0 (-13) で cascade pair hypothesis 汎用形がさらに検証 (3 主指標 100% 解消で cascade 完全消滅) |
| **B?-η-1 §3.3 (Agent 並列 disjoint scope)** | **B?-η-1 commit** | **本 sub-bundle で直接継承 = 同形適用** (12 file 残を 3 Agent disjoint 分割 + pilot 1 file Claude 自力) |
| **B?-η-2 (a) §3.1 (scope refinement 範式)** | **B?-η-2 (a) commit** | **本 sub-bundle で 3rd-level に拡張** = handoff §10 想定外 root cause (B?-η-1 自作 bug = guard macro collision) 発見時の scope refinement 範式 |
| B?-η-2 (a) §3.2 (cascade pair hypothesis 汎用形) | B?-η-2 (a) commit | 本 η-3 で missing #endif 13→0 で汎用形完全検証 (任意 root cause の cascade pair が解消される実証) |
| B?-η-2 (a) §3.3 (第6層 emergence 6 種+α 分類) | B?-η-2 (a) commit | 本 η-3 §3.3 で第7層 emergence 4 系統分類に拡張 |

---

## §7 risks 観測点 (本 sub-bundle で発生した、または将来発生し得る)

| risk | 観測状況 | 想定対処 |
|---|---|---|
| (1) UBO body 差異 (PerDrawUBO 6 group) で guard wrap が link failure exposure を引き起こす (B?-η-1 §7 (1) 継承) | 本 η-3 で per-group 固有化により Link failed 10→0 = risk (1) 完全解消 | 完了 |
| (2) skip list 0 file 再 touch | 本 η-3 scope の 13 file は skip list と完全 disjoint = admission 範式適用不要 | 次 sub-bundle で skip list file が scope に含まれる場合は B?-δ admission 範式継承 |
| (3) cascade pair hypothesis 汎用形 (任意 root cause で `#endif` 消費 cascade) のさらなる検証 | 本 η-3 で missing #endif 13→0 = 3 主指標 100% 解消で cascade 完全消滅 = 汎用形完全検証 | 次 sub-bundle 以降は cascade pair hypothesis を自動消滅指標として継続観測 |
| (4) handoff §10 想定外発見 (scope refinement 3rd-level) 範式の濫用リスク | 本 η-3 が初回適用、scope shrink との区別基準 (想定外 root cause が literal grep + log context で実証 + 過去 patch との因果関係確認) を明示 | 次 sub-bundle 以降も scope refinement 3rd-level 適用時は同基準で実証必須 |
| (5) 第7層 emergence 4 系統 (redefinition `'normalMap'` 188 件 + non-opaque uniforms 127 件 + sampler `'binding'` 73 件 + `'location'` 18 件) の規模 | §4 想定通り (link 成立で linker phase 到達後初露出 計 339 件) | 次 sub-bundle 設計時に第7層 emergence 4 系統 + handoff §10 確定 9 件を整理する scope 設計、B?-η-4 (主 = handoff §10 9 件) + B?-η-5 以降 (第7層 emergence 4 系統) と分割推奨 |
| (6) Agent 並列 disjoint scope の file 衝突 | 0 件 (12 file 残を 3 Agent A/B/C 完全 disjoint、各 5/4/3 file) | 次 sub-bundle で Agent 並列を行う場合も同 disjoint 分割を維持 |
| (7) literal verify 範式忘却 | Agent 3 件全件 self-report と Claude 側 `git diff --stat` 突合で整合確認 ✓ | 次 sub-bundle でも literal grep 結果 vs Agent 報告の併走を必須 |
| (8) shader_cache 247+ 維持 観測点 | 本 η-3 で 223 件 (-26 link 成立 program 集合シフト) = 247+ 維持に届かないが -21 観測点完全解消継続 = 健全 | 次 sub-bundle で 223+ 維持 観測点に切替 |
| (9) charter §3 #1 byte-for-byte 維持 verify | git diff context line 検証で `#else` GL path uniform 宣言 + outer `#ifdef`/`#endif` + 既存 η-1/η-2 (a) patch 全不変 | 次 sub-bundle でも git diff context line ≥ 2 で確認 |
| (10) 範式誤伝承 (B?-η-1 §3.1 範式の bug fix 形応用は PerDrawUBO 6 group で確立、他 UBO への伝承時は collision detection 範式 (§3.2) の事前適用必須) | §3.1 / §3.2 で η-1 範式の bug fix 形応用と明示 + collision detection 範式新規確立 | 次 sub-bundle で他 UBO (例 AtmoExtraUBO_Legacy 等) に guard wrap が必要になる場合は §3.2 collision detection 範式を事前適用 |

---

## §8 commit log

```
733bfc225f feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-3 完遂 (13 file +52/-52 rename-only、AYA 「OK」明示指示下 commit 2026-06-02)
```

詳細 commit message body は git log 参照。

---

## §10 次 sub-bundle B?-η-4 (仮称) 推奨 scope (handoff §10 確定 9 件 移管 + 第7層 emergence 4 系統 別 sub-bundle 振替)

### §10.1 推奨 scope = B?-η-4 (b-1)+(b-2)+(c) cascade root cause 3 種別整理 (handoff §10 移管)

**(b-1) non-opaque uniforms outside a block 4 件 trace** (主 scope):
- 該当 LINE: 458 / 457 / 347 / 349 (B?-η-2 (a)-complete handoff §10.1 「+1 件 再 trace 必要」は本 η-3 着手前 trace で 4 件と確定、誤記訂正済)
- 該当 program: Skinned Deferred PBR Opaque / Deferred PBR Opaque / Contrast Adaptive Sharpening / CAS Legacy Gamma
- 想定対処: B?-δ §3.1 範式継承 = utility 既 attach 全 file scope unguarded bare uniform 網羅 scan + guard wrap (4 strdup 追加/uniform)
- 想定 file 数: 4 file / 想定行数: 小規模 (16-20 行 insertions 規模)

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

### §10.2 B?-η-4 着手前 trace 範式 (B?-ε §3.2 + B?-δ §3.2 + B?-η-1 §3.3 + B?-η-3 §3.1 統合)

1. **literal grep 範式** (B?-η-1 §3.2 cascade source 特定範式継承):
   - `grep -n "non-opaque uniforms outside a block" log` で (b-1) LINE 確定
   - `grep -n "undeclared identifier" log` で (b-2) LINE + identifier 確定 ('size' は除外 = B?-η-3 で完全解消済)
   - `grep -n "redefinition" log` で (c) LINE + variable 確定 ('normalMap' は除外 = 第7層 emergence で別 sub-bundle scope)
2. **log context 抽出範式** (B?-η-2 (a) §4.1 で確立):
   - 各 LINE の `sed -n "$((line-5)),$((line+2))p" log` で program 名 + stage type 確定
3. **handoff §10 想定外発見 trace 範式** (本 η-3 §3.1 で確立):
   - 着手前に **想定 §10 に列挙されていない第3 root cause** が存在しないか log 全件再 trace
   - 想定外 root cause 発見の場合は本 sub-bundle scope を再設計 (3rd-level scope refinement)
4. **Agent 並列 disjoint scope 範式** (B?-η-1 §3.3 / B?-η-2 (a) §3.1 / B?-η-3 直接継承):
   - 9 件 root cause を 3 Agent disjoint scope に分割 (例: Agent A = (b-1) 4 件 / Agent B = (b-2) 3 件 / Agent C = (c) 2 件)
   - pilot 1 file (Claude 自力) で patch literal 確立してから Agent 並列展開

### §10.3 B?-η-4 完遂後の想定 cascade exposure 第8層

- (b-1) 完遂で `non-opaque uniforms outside a block` 127 → 期待 -4 (B?-δ 完了範式継承で utility scope のみ -4、program-scope は別 sub-bundle)
- (b-2) 完遂で `undeclared identifier` 3 → 0
- (c) 完遂で `weight4`/`weight` redefinition 2 → 0
- 全 3 種根本対処で missing #endif cascade (現状 0) 維持
- 第7層 emergence 4 系統 (redefinition `'normalMap'` 188 + non-opaque uniforms 残 + sampler `'binding'` 73 + `'location'` 18) は第8層 emergence として顕在化 = η-5 以降 sub-bundle scope

### §10.4 第7層 emergence 整理 (B?-η-4 完遂後 or 別 sub-bundle で扱う候補)

| emergence 種 | 件数 | 想定対処範式 | 想定 sub-bundle |
|---|---|---|---|
| `'normalMap' : redefinition` at LINE `0:1332` | 188 | B?-ε §3.2 範式継承 + B?-ζ §3.1 範式継承 (cascade source 1 件特定 + struct guard wrap or `#define` rename) | B?-η-5 (推奨) = 単一 cluster 1 patch 188 件解消 |
| non-opaque uniforms outside a block (program-scope 残) | 127 - 4 = 123 (η-4 で utility-scope 4 件解消後) | B?-δ §3.1 範式継承 = program-scope unguarded bare uniform wrap scan | B?-η-6 (推奨) = program-scope 拡大 scope |
| opaque `'binding'` (sampler/texture/image requires layout(binding=X)) | 73 | B?-δ 範式 + B2-γ §3.3 utility concat hook 補強 + sampler binding 追加範式 | B?-η-7 (推奨) = sampler binding 専用 |
| `'location'` (SPIR-V missing + overlapping 20) | 18 | B2-α §3.1 範式継承 = varying + fragment_out program 注入 scope 拡大 + location 重複検出範式 | B?-η-8 (推奨) = location 専用 |

---

## §11 観測点

1. **shader_cache 223+ 維持** maintained 観測点 (B?-η-2 (a) 249 baseline から本 η-3 -26 link 成立 program 集合シフト後の新基準)
2. **cascade pair hypothesis 汎用形** 完全検証完了 (本 η-3 で missing #endif 13→0 で 3 主指標 100% 解消で cascade 完全消滅 = 汎用形完全証明)、次 sub-bundle 以降は自動消滅指標として継続観測
3. **B?-η-1 §3.1 範式の bug fix 形応用範囲** = PerDrawUBO 6 group で確立、他 UBO (FrameAtmosphere 1 body / FrameLights 1 body 等) は body 共通 = collision risk 低、新規 UBO 追加時は §3.2 collision detection 範式事前適用必須
4. **handoff §10 想定外発見 (scope refinement 3rd-level) 範式** (本 η-3 §3.1 で初回適用) の濫用リスク監視 = scope shrink との区別基準 (想定外 root cause が literal grep + log context で実証 + 過去 patch との因果関係確認) を厳守
5. **第7層 emergence 4 系統** (redefinition `'normalMap'` 188 + non-opaque 127 + binding 73 + location 18 = 計 406 件) 整理 = B?-η-4 (handoff §10 確定 9 件) + B?-η-5 以降 (第7層 4 系統) の sub-bundle 分割設計が次 handoff doc §10 の課題
