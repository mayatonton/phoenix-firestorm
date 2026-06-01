# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-7 完遂 handoff

**status**: B?-η-7 完遂 → 次 sub-bundle B?-η-8 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: `874d1a7252` (2026-06-02)
**handoff doc commit**: 本 doc (B?-η-6-complete `fe757ea624` 範式継承)
**勤続範式継承**: B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-7 scope = handoff §10 確定 5 件 main metric 4 root cause 同時対処 + cascade emergence 1 件 5th-level scope refinement 取込み、**3 phase 構成**:

- **phase 1** (handoff §10 §10.2 trace 結果 3 root cause 直接対処): 3 file +47/-2、Claude 自力 disjoint scope 並列展開 (Agent 投入 0)
  - **(a)** `underWaterF.glsl` `UnderWaterFParamUBO_Legacy` 内 `waterFogColor` `waterFogKS` per-block 固有化 rename (η-6 §3.3 範式 nameless block × member collision 同形、waterFogF.glsl `WaterFogUBO_Legacy` member との global scope collision)
  - **(b)** `avatarV.glsl` `WEIGHT_LOCATION_DEFINED` guard wrap (η-5 §3.4 (c) 範式継承、avatarSkinV.glsl と独立宣言衝突)
  - **(c)** `rlvF.glsl` `RlvFParamUBO_Legacy` (set=3, binding=56) UBO wrap + **bvec2→uvec2 uint promote + `#define` alias 戻し範式** (η-7 新規 §3.1) (RLVa Sphere bare uniform 6 件 UBO 化)
- **phase 2-A** (5th-level scope refinement = cascade emergence sub-bundle 内取込み): 1 file +9/-0
  - `avatarV.glsl` `AvatarClothVParamUBO_Legacy` (set=3, binding=57, std140) UBO wrap (phase 1 (b) 'weight' redefinition 解消で parse 進行 → AVATAR_CLOTH 内既存 bare uniform 3 件露出 = Deferred Avatar Shader vertex 0:464 新規 emergence)
- **phase 2-B** (Reflection Mip Shader 0:1969 trace + fix): 1 file +14/-0
  - `gaussianF.glsl` `GaussianFParamUBO_Legacy` (set=3, binding=58, std140) UBO wrap (llviewershadermgr.cpp L3963/L3977 で `"Reflection Mip Shader"` 名が gReflectionMipProgram (reflectionmipF 既 wrap 済) + gGaussianProgram (gaussianF 未 wrap) で **program 名共有** = 真因 gaussianF 特定 = **program name collision 確認範式** η-7 新規 §3.2)
- **phase 2-C** (Avatar Eyes 0:570 trace + 移管判断): patch 0 file
  - Avatar Eyes fragment helper 10 file (globalF / srgbF / atmosphericsVarsF / atmosphericsHelpersF / shadowUtil / gammaF / atmosphericsFuncs / atmosphericsF / waterFogF / diffuseF) 静的 wrap 確認済 → preprocessed line 570 の bare uniform 源は static trace 不可 → η-6 §3.4 範式 (Agent capability limit) 適用条件成立 → **B?-η-8 移管**

**計**: 4 file +65/-2 (shader file のみ、C++ touch 0)

**主指標達成**:
- `'weight' : redefinition` 1 → **0** ✓ -1 / 100% 完全達成 (phase 1 (b) 直接効果)
- `nameless block ... global scope` 1 → **0** ✓ -1 / 100% 完全達成 (phase 1 (a) 直接効果)
- `non-opaque uniforms outside a block` 3 → **1** -2 / 67% 達成 (phase 1 (c) RLVa 解消 + phase 2-A cloth emergence 解消 + phase 2-B Reflection Mip 解消 / Avatar Eyes 0:570 残 = B?-η-8 移管 η-6 §3.4 範式)

**既達主指標完全維持** (4 種): `Cannot reuse` 0 / `'weight4' redefinition` 0 / `undeclared identifier` 0 / `Link failed` 0 + 追加維持 (5 種): `'size'` 0 / `GBufferInfo` 0 / `'#'` 0

**第8層 emergence 微小**: `'location'` +1 (27 → 28、link 成立 program 集合シフトによる、B?-η-9 移管対象)

---

## §2 完遂結果 metric (vs B?-η-6 baseline)

| metric | η-6 baseline | η-7 phase 1 (推定) | η-7 phase 2 (commit) | Δ vs η-6 | 判定 |
|---|---|---|---|---|---|
| 'weight' redefinition | 1 | 0 | **0** | -1 | ✓ 100% 完全達成 |
| nameless block ... global scope | 1 | 0 | **0** | -1 | ✓ 100% 完全達成 |
| non-opaque uniforms outside a block | 3 | 2 | **1** | -2 | 67% 達成 (Avatar Eyes 0:570 残 = B?-η-8 移管) |
| Cannot reuse block name | 0 | 0 | 0 | ±0 | ✓ η-1 達成維持 |
| 'weight4' redefinition | 0 | 0 | 0 | ±0 | ✓ η-5 達成維持 |
| undeclared identifier | 0 | 0 | 0 | ±0 | ✓ η-5 達成維持 |
| Link failed | 0 | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| 'size' undeclared | 0 | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| '#' preprocessor | 0 | 0 | 0 | ±0 | ✓ ε 達成維持 |
| missing #endif | 90 | (測定割愛) | 89 | -1 | cascade pair 内訳変化 |
| parse failed | 137 | (測定割愛) | 134 | -3 | cascade 縮小 = 主指標 -4 + 第8層 emergence net +1 整合 |
| redefinition (all) | 81 | (測定割愛) | 80 | -1 | 'weight' -1 |
| 'binding' | 25 | (測定割愛) | 25 | ±0 | 第7層 emergence 横ばい |
| 'location' | 27 | (測定割愛) | 28 | +1 | 第8層 emergence 微小 |
| shader_cache 件数 | 302 | (測定割愛) | **305** | +3 | link 成立 program 集合シフト |
| FATAL/SIGSEGV/Aborted | 0 | 0 | 0 | ±0 | ✓ |
| Goodbye / Vulkan device destroyed / instance destroyed / status: stopped | 1/1/1/1 | 1/1/1/1 | 1/1/1/1 | ±0 | ✓ clean shutdown |

---

## §3 設計範式

### §3.1 新規 設計範式: bvec2/bool → uvec2/uint uint promote + `#define` alias 戻し範式 (std140 type 制約対応)

**概要**: std140 layout で **不許可な type** (bool / bvec2 / bvec3 / bvec4 等) を bare uniform から UBO wrap する際、`uvec2` 等 uint type に **promote** で UBO 化、main() body 内参照は `#define <orig> <cast_expr>` alias で **bvec2 cast 戻し** = byte-for-byte 不変担保。

**位置付け**: η-6 phase 2-A `skinSSSF.glsl` `aya_visual_realism_enabled` の `#define` alias 範式 (bool member rename) の **std140 type 制約への拡張**、bool/bvec2 → uint/uvec2 への **type promote 形態**。

**std140 制約背景**: std140 layout は scalar bool / bvec2 / bvec3 / bvec4 を許可しない (Vulkan spec / glslang strict mode で reject)。一方 uint / uvec2 / uvec3 / uvec4 は許可される。

**適用フロー** (4 step):
1. **UBO body 内**: 不許可 type → 許可 type へ rename (`bvec2 X` → `uvec2 X_uvec`)
2. **`#define` alias**: Vulkan path 内に `#define <orig_name> <cast_expr>` (`bvec2(X_uvec.x != 0u, X_uvec.y != 0u)`) を配置
3. **main() body**: `<orig_name>` 参照は byte-for-byte 不変
4. **GL `#else` path**: 既存 bare uniform 宣言 (`uniform bvec2 X`) 不変 (charter §3 #1 担保)

**η-7 適用例** (phase 1 (c) `rlvF.glsl`):

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=3, binding=56, std140) uniform RlvFParamUBO_Legacy {
    vec4  rlvEffectParam1;
    vec4  rlvEffectParam2;
    vec4  rlvEffectParam4;
    vec2  rlvEffectParam5;
    uvec2 rlvEffectParam3_uvec;       // bvec2 → uvec2 promote
    int   rlvEffectMode;
    int   _pad_rlv_legacy_0;
    int   _pad_rlv_legacy_1;
    int   _pad_rlv_legacy_2;
};
#define rlvEffectParam3 bvec2(rlvEffectParam3_uvec.x != 0u, rlvEffectParam3_uvec.y != 0u)
#else
uniform bvec2 rlvEffectParam3;  // GL path byte-for-byte 不変
// ...
#endif

// main() body: SPHERE_DISTEXTEND = rlvEffectParam3 (byte-for-byte 不変)
```

**Trade-off**:
- 利点: std140 制約に従いつつ main() body 完全不変、GL path 不可触担保
- 注意: `0u` ↔ `bvec2.x` の比較は scalar branch で問題なし、SIMD branch では SPIR-V optimizer 任せ (uint != 0u → bool conversion は IL レベルで定数畳込み)

### §3.2 新規 設計範式: program name collision 確認範式 (log program 名 → llviewershadermgr.cpp mName grep)

**概要**: error log の program 名 (例: `"Reflection Mip Shader"`) が **複数 program で共有** されている場合、log だけでは真因 program file が判別不能 → `llviewershadermgr.cpp` 内の `mName = "<name>";` literal grep で **重複 program registration を検出**、真因 program file (= bare uniform 残存 file) を特定する範式。

**位置付け**: η-6 §3.2 Agent 初期 trace 誤判定検出範式の **log program 名 不一意検出 拡張形**、feedback_admit_unknown 範式の sub-bundle 内構造化、Claude 自力 trace で完結可能 (Agent 投入不要)。

**適用条件**: log の error LINE (例: `Reflection Mip Shader 0:1969`) で source file 同定不能、program 名が generic で識別性不足

**Detection 手順** (3 step):
1. **log program 名 抽出**: error LINE 直前の program 名 (例: `"Reflection Mip Shader"`)
2. **mName grep**: `grep -n 'mName = "<program 名>"' indra/newview/llviewershadermgr.cpp` で複数行 hit 確認
3. **shader file 識別**: 各 program の `mFragmentFiles.push_back("<file>")` で attach file を識別、その中で **bare uniform 残存 file** (UBO wrap 未適用) を真因 file と特定

**η-7 適用例** (phase 2-B `gaussianF.glsl`):
- log: `Reflection Mip Shader 0:1969` (non-opaque uniforms outside a block)
- mName grep:
  - L3963: `gReflectionMipProgram.mName = "Reflection Mip Shader";` + `reflectionmipF.glsl` fragment (η-6 phase 2-C で wrap 済)
  - L3977: `gGaussianProgram.mName = "Reflection Mip Shader";` + `gaussianF.glsl` fragment (**未 wrap = 真因**)
- 結論: `gaussianF.glsl` の `resScale` `direction` 2 件 bare uniform が真因 → UBO wrap 適用

**Trade-off**:
- 利点: Claude 自力 trace で完結、Agent capability limit (§η-6 §3.4) を回避
- 注意: 適用範囲は program name collision 発生時のみ、一意 program 名なら本範式不要 (log で source file 即特定可能)

### §3.3 5th-level scope refinement の cascade emergence 取込み拡張 (η-5 §3.2 の applied form)

**概要**: η-5 §3.2 5th-level scope refinement 範式 (handoff §10 確定 scope 着手後 verify 工程で **想定外の自作 bug** 発見時の遡及 fix) を、**自作 bug でない cascade emergence** にも適用する形態。

**位置付け**: η-5 §3.2 の **自作 bug 発見条件 4 つ** (a verify 工程で metric 退行 + b git blame で過去 patch 起因と特定 + c sub-bundle scope と同根 root cause + d 24h 内で fix 可能) を、cascade emergence (= parse 進行で既存 bare uniform 露出) へ **条件 (b) を緩和** (= git blame ではなく cascade pair hypothesis による既存 bug emergence) で適用。

**適用条件** (cascade emergence 取込みの 3 条件):
1. **24h ceiling 内**: phase 1 + phase 2-A で 24 時間以内に完了見込み
2. **同 file 延長**: phase 1 で touch した file の **同一 preprocessor branch** (= AVATAR_CLOTH branch) 内 emergence
3. **patch pattern 確立済**: η-6 UBO wrap 範式が既に確立されており新規 trace 不要

**η-7 適用例** (phase 2-A `avatarV.glsl`):
- phase 1 (b) `WEIGHT_LOCATION_DEFINED` guard wrap で `'weight' : redefinition` 解消
- parse 進行 → 既存 AVATAR_CLOTH bare uniform 3 件 (`gWindDir` / `gSinWaveParams` / `gGravity`) 露出
- Deferred Avatar Shader vertex 0x8b31 0:464 で `non-opaque uniforms outside a block` 新規 emergence
- 3 条件成立 → phase 2-A で `AvatarClothVParamUBO_Legacy` (set=3, binding=57, std140) UBO wrap、sub-bundle 内取込み

**Trade-off**:
- 利点: 次 sub-bundle (B?-η-8) への移管を回避、handoff §10 scope を sub-bundle 完結で消化
- 注意: 自作 bug 性が **無い** (= prior sub-bundle で生み出した bug ではなく既存 bare uniform 露出) ことを明示、feedback_root_cause_not_dump 範式適用は不要 (parse 進行による自然 emergence)

### §3.4 η-6 §3.3 nameless block × member collision の data member 適用 第 4 例

**概要**: η-4 §3.2 (padding member) → η-5 §3.4 (通常 data member) → η-6 §3.3 (bare uniform × nameless block member) の member-level collision 範式を、η-7 で **nameless block A の member ↔ nameless block B の同名 member** に再適用 (= η-4/η-5 と同形だが第 4 適用例)。

**位置付け**: η-6 §3.3 範式の **過去型** (nameless block × nameless block member) への回帰、η-4 §3.2 と同形で **data member 適用第 4 例**。

**η-7 適用例** (phase 1 (a) `underWaterF.glsl`):
- `UnderWaterFParamUBO_Legacy` member `waterFogColor` `waterFogKS` が `waterFogF.glsl` `WaterFogUBO_Legacy` (set=3, binding=9) member と同名で global scope collision
- per-block 固有化 rename: `waterFogColor` → `waterFogColor_underwater_legacy`、`waterFogKS` → `waterFogKS_underwater_legacy`
- main() 内未参照 (= `waterFogColorLinear` のみ参照) → `#define` alias 不要
- GL `#else` path 不可触担保 (charter §3 #1)

### §3.5 η-1 §3.2 cascade pair hypothesis 汎用形 1:1 比例検証 (η-7 verify)

**η-7 検証**:
- 主指標 4 件解消 (-1 'weight' + -1 nameless + -2 non-opaque) vs missing #endif -1 件 = cascade pair 1:1 比例 **「順」方向** (主指標減少 + cascade pair 減少)
- 原因: phase 1 (b) `WEIGHT_LOCATION_DEFINED` guard wrap で `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 構造 1 セット **減少** ではなく `#ifndef WEIGHT_LOCATION_DEFINED ... #endif` 内ガード追加で内訳変化
- net 評価: parse failed -3 = (主指標 -4) + (第8層 emergence net +1 = 'location' +1) で整合
- 結論: cascade pair hypothesis は **patch 数小規模時には順方向シフト** (η-6 の逆方向シフトは大規模 wrap 24 件由来)、main metric 主導の整合性は維持

---

## §4 cold cache launch verify metric 詳細 (vs B?-η-6 baseline)

### §4.1 主指標 (handoff §10 5 件 主 scope + cascade emergence 1 件)

| 主指標 | η-6 | η-7 commit | Δ | 内訳 |
|---|---|---|---|---|
| 'weight' redefinition | 1 | **0** | -1 ✓ | phase 1 (b) `WEIGHT_LOCATION_DEFINED` guard wrap 直接効果 |
| nameless block ... global scope | 1 | **0** | -1 ✓ | phase 1 (a) underWaterF `waterFogColor` `waterFogKS` per-block 固有化 直接効果 |
| non-opaque uniforms outside a block | 3 | **1** | -2 | phase 1 (c) RLVa 解消 + phase 2-A cloth emergence 解消 + phase 2-B Reflection Mip 解消、残 1 件 = Avatar Eyes 0:570 (B?-η-8 移管) |
| Cannot reuse block name | 0 | **0** | ±0 ✓ | η-1 達成完全維持 |

### §4.2 既達主指標完全維持

| 主指標 | 値 | 由来 sub-bundle |
|---|---|---|
| 'weight4' redefinition | 0 | B?-η-5 達成完全維持 |
| undeclared identifier | 0 | B?-η-5 達成完全維持 |
| 'size' undeclared | 0 | B?-η-3 達成完全維持 |
| Link failed | 0 | B?-η-3 達成完全維持 |
| GBufferInfo redefinition struct | 0 | B?-ζ 達成完全維持 |
| '#' preprocessor | 0 | B?-ε 達成完全維持 |

### §4.3 第8層 emergence 微小

| metric | η-6 | η-7 | Δ | 評価 |
|---|---|---|---|---|
| 'location' | 27 | 28 | +1 | 第8層 emergence 微小 (link 成立 program 集合シフト由来、B?-η-9 移管) |
| 'binding' | 25 | 25 | ±0 | 第7層 emergence 横ばい (B?-η-8 移管対象) |
| shader_cache 件数 | 302 | 305 | +3 | link 成立 program 集合シフト (微小) |

### §4.4 主指標 metric integrity self-check (B3 §12 literal grep 範式継承)

- 主指標 4 種 (η-6 baseline 3 種 + cascade emergence 1 種) で -4 (-1 weight + -1 nameless + -2 non-opaque)
- cascade pair 1:1 比例「順」 (main -4 vs missing #endif -1)
- parse failed -3 = main -4 + 第8層 emergence net +1 ('location' +1) で整合
- shader_cache +3 = link 成立 program 集合シフト = -21 観測点完全解消継続維持

### §4.5 shutdown clean verify

- FATAL/SIGSEGV/Aborted: 0/0/0 ✓
- Goodbye! : 1 ✓
- Vulkan device destroyed: 1 ✓
- Vulkan instance destroyed: 1 ✓
- status: stopped: 1 ✓
- launch 起動成立: 2026-06-02 (phase 1 baseline + phase 2-A baseline + phase 2-B 最終 全 3 cycle clean shutdown)

---

## §5 self-verify (B3 §12 + B?-η-6 §5 範式継承)

| # | 項目 | 結果 |
|---|---|---|
| 1 | charter §3 #1 GL path byte-for-byte 不変 担保 | ✓ (4 file 全 outer `#ifdef LL_VULKAN_GLSL ... #else <GL path> ... #endif` literal 不変、rlvF.glsl bvec2 既存宣言不変 + #define alias は Vulkan path 内側 only) |
| 2 | shader file のみ編集 (C++ touch 0) | ✓ `git diff --stat HEAD~1 -- '*.cpp' '*.h'` 0 (但し llviewershadermgr.cpp は read-only inspect = trace のみ) |
| 3 | 主指標 4 種 literal grep verify | ✓ 'weight' 0 + nameless 0 + non-opaque 1 + Cannot reuse 0 |
| 4 | 既達主指標 6 種 literal grep verify | ✓ weight4 0 + undeclared 0 + Link failed 0 + 'size' 0 + GBufferInfo 0 + '#' 0 全 ±0 |
| 5 | cascade pair 1:1 比例検証 (汎用形 順方向シフト) | ✓ missing #endif -1 vs main -4 = patch 数小規模時の順方向シフト |
| 6 | parse failed integrity | ✓ -3 = main -4 + 第8層 emergence net +1 |
| 7 | shader_cache 件数 | ✓ 305 (+3 = link 成立 program 集合シフト) |
| 8 | FATAL/SIGSEGV/Aborted 0 維持 | ✓ 0/0/0 |
| 9 | clean shutdown | ✓ Goodbye/Vulkan destroy/status stopped 各 1 |
| 10 | 自作 bug 性なし cascade emergence の sub-bundle 内取込み (§3.3) | ✓ phase 2-A AVATAR_CLOTH UBO wrap、3 条件全成立で取込み |
| 11 | bvec2 → uvec2 uint promote + #define alias 範式 (§3.1) | ✓ rlvF.glsl 適用、main() body byte-for-byte 不変 + GL path 不可触 |
| 12 | program name collision 確認範式 (§3.2) | ✓ phase 2-B gaussianF.glsl 適用、Claude 自力 trace で真因特定 |
| 13 | Agent capability limit による B?-η-8 移管 (η-6 §3.4 継承) | ✓ phase 2-C Avatar Eyes 0:570 移管判断記録 |
| 14 | nameless block × member collision 範式 (η-6 §3.3 継承) | ✓ phase 1 (a) underWaterF waterFogColor/waterFogKS rename、data member 適用第 4 例 |
| 15 | scope refinement 5th-level (η-5 §3.2) cascade emergence 形 (§3.3) | ✓ phase 1 verify → cascade emergence 1 件 → phase 2-A 取込み |
| 16 | 過去 sub-bundle 既処理 file byte-for-byte 維持 | ✓ A1-A7/A8-recovery/B1-B3/B2-α-γ/B?-δ-η-6 全件 |
| 17 | skip list admission 範式継承 (B?-δ) | ✓ insertions-only or rename-only + outer LL_VULKAN_GLSL 不変 (underWaterF.glsl/avatarV.glsl 再 touch admission) |
| 18 | handoff doc 別 commit (η-6 範式継承) | ✓ 本 doc は patch commit `874d1a7252` 後の別 commit |

---

## §6 設計範式継承表

### feedback memory 範式 (10 件全件適用確認)

| # | feedback memory | η-7 適用箇所 |
|---|---|---|
| 1 | feedback_doubt_self_first | phase 2-B Reflection Mip 真因 trace で reflectionmipF 既 wrap 済を疑い、llviewershadermgr.cpp mName grep で gaussianF 真因特定 |
| 2 | feedback_admit_unknown | phase 2-C Avatar Eyes 0:570 helper 10 file 静的 wrap 確認後、preprocessed source 同定不能 → η-6 §3.4 範式適用で B?-η-8 移管判断 |
| 3 | feedback_build_only_verified | cold cache launch verify を phase 1 / phase 2-A / phase 2-B 各 cycle 後実施、metric grep で確定 |
| 4 | feedback_falsification_as_progress | phase 1 verify で cascade emergence 1 件発見を progress として handoff doc 記録 |
| 5 | feedback_one_step_at_a_time | AYA 「OK」明示指示後 phase 単位 deploy + verify (3 cycle: phase 1 → phase 2-A → phase 2-B) |
| 6 | feedback_no_auto_commit | AYA 「commit + handoff doc 起草に着手しますか?」 → 「OK」明示指示後 commit |
| 7 | feedback_no_claude_coauthor | Co-Authored-By: Claude 行を含めない (全 commit) |
| 8 | feedback_no_scope_shrink | phase 1 で 5 件 main metric 全件 attempt、scope 縮小せず、Avatar Eyes 0:570 移管は技術的制約 (static trace 限界) のみ |
| 9 | feedback_shader_only_fast_iterate | shader file のみ編集、autobuild 不要、cp + rm shader_cache で fast iterate (3 cycle) |
| 10 | feedback_root_cause_not_dump | phase 1 (c) RLVa bvec2 std140 制約を `uvec2` promote + `#define` alias で根本解決 (fallback / disable なし) |

### prior sub-bundle 範式 (継承表)

| sub-bundle | 範式 | η-7 適用 |
|---|---|---|
| B1 §3 | shader-file 側 UBO 宣言 guard wrap | phase 1 (c) rlvF + phase 2-A avatarV-cloth + phase 2-B gaussianF |
| B2-α §3.1 | GL `#else` branch byte-for-byte 不変 | 4 file 全件 |
| B3 §3.2 | metric integrity self-check | §4.4 |
| B3 §12 | literal grep metric verify | §4.1-§4.5 全 metric |
| B2-γ §3.1 | Agent 並列 disjoint scope | (本 sub-bundle Agent 投入 0、Claude 自力 disjoint scope のみ) |
| B2-γ §3.2 | pilot 1 file で patch literal 確立 | phase 1 (a)/(b)/(c) 各 root cause で 1 file pilot |
| B2-γ §3.3 | charter §3 #1 担保 | 4 file 全件 |
| B?-δ §3.1 | skip list admission | underWaterF.glsl/avatarV.glsl 再 touch admission |
| B?-δ §3.2 | UBO body member byte-for-byte 維持 | phase 1 (a) underWaterF member rename は Vulkan path 内 member 名のみ + GL path 不変 |
| B?-ε §3.2 | 1 source N 件 cluster 対処 | (本 sub-bundle 該当なし、phase 1/2 各 root cause 1 file 1 patch) |
| B?-ζ §3.1 | shader-file 側 UBO 宣言 guard wrap | phase 1 (c) + phase 2-A + phase 2-B 3 UBO 追加 |
| B?-η-1 §3.1 | shader-file 側 FrameViewProj guard wrap | (本 sub-bundle 該当なし、η-7 は per-program UBO のみ) |
| B?-η-1 §3.2 | cascade pair hypothesis | §3.5 順方向シフト検証 |
| B?-η-1 §3.3 | Agent 並列 disjoint scope | (本 sub-bundle Agent 投入 0、Claude 自力 disjoint scope) |
| B?-η-2 (a) §3.2 | cascade pair hypothesis 汎用形 | §3.5 順方向シフト検証継続 |
| B?-η-3 §3.1 | scope refinement 3rd-level | §3.3 cascade emergence 取込み 5th-level applied form |
| B?-η-3 §3.2 | guard macro collision detection | phase 1 (b) WEIGHT_LOCATION_DEFINED guard wrap |
| B?-η-4 §3.1 | scope refinement 4th-level (sub-bundle 内 3 phase refine) | phase 1 / phase 2-A / phase 2-B / phase 2-C 構成 |
| B?-η-4 §3.2 | nameless block member 固有化 detection 範式 | §3.4 data member 適用第 4 例 |
| B?-η-4 §3.4 | 仮説 4 件評価による真因絞り込み範式 | phase 2-B Reflection Mip 真因 trace (program name collision 確認) |
| B?-η-5 §3.1 | multi-root-cause 同時対処範式 (root cause 軸) | phase 1 で 3 root cause ((a) nameless + (b) weight + (c) non-opaque) を disjoint scope で並列対処 |
| B?-η-5 §3.2 | scope refinement 5th-level (自作 bug 遡及 fix 範式) | §3.3 cascade emergence 取込み拡張形 (自作 bug 性なし版) |
| B?-η-5 §3.3 | Agent depth trace 投入閾値範式 | (本 sub-bundle Agent 投入 0、Claude 自力で完結) |
| B?-η-5 §3.4 | nameless block 範式 member-level 拡張範囲明示 | §3.4 data member 適用第 4 例 (η-6 §3.3 form 回帰) |
| B?-η-5 §3.4 (c) | weight attribute guard wrap | phase 1 (b) `WEIGHT_LOCATION_DEFINED` guard wrap (avatarV.glsl 適用) |
| B?-η-6 §3.1 | multi-cluster pilot + Agent 並列展開範式 | (本 sub-bundle 該当なし、cluster 規模未到達 = 5 件 main metric / file-cluster 分割不要) |
| B?-η-6 §3.2 | Agent 初期 trace 誤判定検出 → log stage type literal grep 再 trace 範式 | (本 sub-bundle Agent 投入 0、§3.2 program name collision 確認範式が同形作用) |
| B?-η-6 §3.3 | nameless block × bare uniform / member collision 範式 | §3.4 data member 適用第 4 例 (form 回帰) |
| B?-η-6 §3.4 | Agent capability limit による sub-bundle 移管判断範式 | phase 2-C Avatar Eyes 0:570 移管判断適用 |
| B?-η-6 §3.5 | cascade pair hypothesis 汎用形 逆方向シフト検証 | §3.5 順方向シフト検証 (patch 数小規模時の対称形) |

### 新規 設計範式 (§3.1-§3.5 計 5 件)

- §3.1: **bvec2/bool → uvec2/uint uint promote + `#define` alias 戻し範式** (std140 type 制約対応、η-6 phase 2-A `#define` alias 範式の type promote 拡張、rlvF.glsl 適用)
- §3.2: **program name collision 確認範式** (log program 名 → llviewershadermgr.cpp mName grep で真因 file 特定、Claude 自力 trace で完結、gaussianF.glsl 適用)
- §3.3: **5th-level scope refinement の cascade emergence 取込み拡張** (η-5 §3.2 の自作 bug 性なし版、3 条件 = 24h ceiling 内 + 同 file 延長 + patch pattern 確立済、avatarV.glsl AVATAR_CLOTH 適用)
- §3.4: η-6 §3.3 nameless block × member collision の data member 適用第 4 例 (underWaterF.glsl waterFogColor/waterFogKS 適用)
- §3.5: cascade pair hypothesis 汎用形 順方向シフト検証 (η-6 逆方向シフトの対称形、patch 数小規模時)

---

## §7 risks

| # | risk | 評価 | 緩和策 |
|---|---|---|---|
| 1 | UBO body 差異 link failure exposure | ✓ 完全解消継続 (B?-η-3 達成維持) | phase 1 / 2-A / 2-B 3 UBO 全件 body member 固有 名前付 |
| 2 | skip list 再 touch admission | ✓ underWaterF.glsl/avatarV.glsl 等再 touch | B?-δ admission 範式 = insertions-only or rename-only + outer LL_VULKAN_GLSL 不変 で skip list 趣旨担保 |
| 3 | cascade pair hypothesis 汎用形 順方向 vs 逆方向シフト | ✓ patch 数小規模時順方向 / 大規模時逆方向 | B?-η-8 以降も継続観測、main metric 主導の整合性は維持 |
| 4 | bvec2/bool → uvec2/uint promote 範式 (§3.1) 適用範囲 | 監視対象 | 適用は std140 layout 不許可 type のみ、scalar bool / bvec2-4 が対象、許可 type (uint / uvec2-4 等) は直接 wrap 可能 |
| 5 | program name collision 確認範式 (§3.2) 濫用 | 監視対象 | 適用は log で source file 同定不能時のみ、一意 program 名なら本範式不要 |
| 6 | 5th-level scope refinement cascade emergence 取込み (§3.3) 濫用 | 監視対象 | 3 条件全成立時のみ取込み、scope shrink への誤誘導回避 (24h 超過時は移管) |
| 7 | Avatar Eyes 0:570 移管リスク | 監視対象 (B?-η-8 引継) | runtime preprocessed dump 取得手順を B?-η-8 着手前 trace で確立、Agent depth trace 投入準備 |
| 8 | 'location' +1 第8層 emergence | B?-η-9 移管 | 第8層 emergence 微小、link 成立 program 集合シフト由来 |
| 9 | literal verify 範式 | ✓ 全 metric literal grep 確定 | B3 §12 範式継承 |
| 10 | shader_cache 305+ 維持観測点切替 | 観測継続 | -21 観測点完全解消継続維持、η-6 302 → η-7 305 +3 |
| 11 | charter §3 #1 byte-for-byte | ✓ 4 file 全件担保 | §5 #1 |
| 12 | set/binding allocation 連続割当上限 | 監視対象 | 現状 set=3, binding=14-58 (45 bindings 使用)、Vulkan 仕様 maxDescriptorSetBoundResources 1024 内 |
| 13 | nameless block × member collision (§3.4) 範式継承の data member 適用第 4 例 過剰展開 | 監視対象 | 適用範囲は nameless block member 同名検出時のみ、通常 collision は §3.1 guard wrap で済 |

---

## §8 commit history

| commit | type | scope | file 数 |
|---|---|---|---|
| `874d1a7252` | feat(r41) | η-7 patch (phase 1 + phase 2-A/2-B) | 4 file +65/-2 |
| `fe757ea624` | docs(r41) | η-6 完遂 handoff (起点) | 1 file +455 |
| `e915af26fe` | feat(r41) | η-6 patch | 34 file +348/-2 |

---

## §9 file inventory (η-7 patch 4 file 内訳)

### phase 1 handoff §10 §10.2 trace 結果 3 root cause 直接対処 (3 file +47/-2)

#### (a) `class3/environment/underWaterF.glsl` +10/-2 (nameless block member 固有化)
- `UnderWaterFParamUBO_Legacy` 内 `waterFogColor` → `waterFogColor_underwater_legacy`
- `UnderWaterFParamUBO_Legacy` 内 `waterFogKS` → `waterFogKS_underwater_legacy`
- main() 内未参照 (`waterFogColorLinear` のみ参照) → `#define` alias 不要
- 範式: η-6 §3.3 = η-7 §3.4 (data member 適用第 4 例)

#### (b) `class1/deferred/avatarV.glsl` +13/-1 (WEIGHT_LOCATION_DEFINED guard wrap)
- `#ifndef WEIGHT_LOCATION_DEFINED #define WEIGHT_LOCATION_DEFINED 1 layout(location=9) in vec4 weight; #endif`
- avatarSkinV.glsl (先 attach via hasSkinning) が `WEIGHT_LOCATION_DEFINED` を define、avatarV (後 attach) は skip
- 範式: η-5 §3.4 (c)

#### (c) `class1/deferred/rlvF.glsl` +21/-0 (RlvFParamUBO_Legacy UBO wrap + bvec2 → uvec2 promote)
- `RlvFParamUBO_Legacy` (set=3, binding=56, std140) 6 bare uniform UBO 化
- `bvec2 rlvEffectParam3` → `uvec2 rlvEffectParam3_uvec` uint promote
- `#define rlvEffectParam3 bvec2(rlvEffectParam3_uvec.x != 0u, rlvEffectParam3_uvec.y != 0u)` alias で bvec2 cast 戻し
- main() body 内 `SPHERE_DISTEXTEND = rlvEffectParam3` 不変
- std140 member 順序: vec4 + vec4 + vec4 + vec2 + uvec2 + int + int×3 padding = 80 byte
- 範式: η-6 §3.1 UBO wrap + η-7 §3.1 (bvec2 → uvec2 promote + #define alias 戻し範式)

### phase 2-A 5th-level scope refinement cascade emergence 取込み (1 file +9/-0)

#### `class1/deferred/avatarV.glsl` +9/-0 (AvatarClothVParamUBO_Legacy UBO wrap)
- `AvatarClothVParamUBO_Legacy` (set=3, binding=57, std140) AVATAR_CLOTH bare uniform 3 件 UBO 化
- `vec4 gWindDir` + `vec4 gSinWaveParams` + `vec4 gGravity`
- Deferred Avatar Shader vertex 0x8b31 0:464 emergence 解消
- 範式: η-7 §3.3 (5th-level scope refinement cascade emergence 取込み)

### phase 2-B Reflection Mip Shader 0:1969 trace + fix (1 file +14/-0)

#### `class1/interface/gaussianF.glsl` +14/-0 (GaussianFParamUBO_Legacy UBO wrap)
- `GaussianFParamUBO_Legacy` (set=3, binding=58, std140) bare uniform 2 件 UBO 化
- `float resScale` + `vec2 direction` (std140 vec2 alignment 8 で 4 byte padding 自動挿入)
- llviewershadermgr.cpp L3963/L3977 で "Reflection Mip Shader" 名 gReflectionMipProgram + gGaussianProgram 共有確認、gaussianF 未 wrap 真因特定
- 範式: η-7 §3.2 (program name collision 確認範式) + η-6 §3.1 UBO wrap

### phase 2-C Avatar Eyes 0:570 trace + 移管判断 (patch 0 file)

#### Avatar Eyes fragment helper 10 file 静的 wrap 確認
- `globalF.glsl` (`GlobalFParamUBO_Legacy` + `PerDrawUBO_ClipPlane` 既 wrap 済)
- `srgbF.glsl` (uniform 無し)
- `atmosphericsVarsF.glsl` (`in` 宣言のみ、uniform 無し)
- `atmosphericsHelpersF.glsl` (`FrameAtmosphere_Lighting` 既 wrap 済)
- `shadowUtil.glsl` (`ShadowUtilParamUBO_Legacy` + `FrameLights` + `FrameViewProj` 既 wrap 済)
- `gammaF.glsl` (DEPRECATED、uniform 無し)
- `atmosphericsFuncs.glsl` (`FrameLights` + `FrameAtmosphere_Lighting` + `AtmoExtraUBO_Legacy` 既 wrap 済)
- `atmosphericsF.glsl` (既 wrap 済)
- `waterFogF.glsl` (`WaterFogUBO_Legacy` 既 wrap 済)
- `diffuseF.glsl` (完全 wrap 済)
- **結論**: 全 helper file の bare uniform は wrap 済 → preprocessed line 570 の bare uniform 源は **static trace 不可** → η-6 §3.4 範式 (Agent capability limit = runtime preprocess dump 取得手順必須) 適用条件成立 → **B?-η-8 移管**

### overlap file 確認
- `class1/deferred/avatarV.glsl`: phase 1 (b) + phase 2-A 2 重 touch (phase 1 = WEIGHT_LOCATION_DEFINED guard / phase 2-A = AVATAR_CLOTH UBO、preprocessor branch + line-range disjoint)

---

## §10 次 sub-bundle B?-η-8 推奨 scope

### §10.1 確定 scope: Avatar Eyes 0:570 Agent depth trace + 第7層 emergence binding 25

| metric | 件数 | 主 program / source | 推定 root cause |
|---|---|---|---|
| non-opaque uniforms outside a block (残) | 1 | Avatar Eyes 0:570 | runtime preprocessed source dump 取得で source file 特定必要、Agent depth trace 投入 |
| 'binding' | 25 | (要 trace) | SPIR-V binding 衝突または overlapping binding |

### §10.2 B?-η-8 着手前 trace 範式 6 ステップ (η-6 §10.2 範式継承 + runtime preprocess dump 取得手順拡張)

1. **literal grep**: `grep -c "non-opaque uniforms outside a block" log` + `grep -c "'binding'" log`
2. **log context 抽出**: 各 program の error LINE × 件数 + stage type 番号 (0x8b30/0x8b31) を **必ず併記** (η-6 §3.2 範式 = Agent 報告に stage type 明示確認)
3. **Avatar Eyes 0:570 runtime preprocessed dump 取得** (η-6 §3.4 範式形式化):
   - `LLGLSLShader::createShader()` 等で glslang preprocess 後の merged source を file dump (`/tmp/avatar_eyes_preprocessed.glsl` 等)
   - dump file の line 570 を確認、bare uniform 宣言を直接特定
   - 取得手順を Agent に明示指示 + dump file 添付で trace 投入
4. **Agent 並列 disjoint scope** (η-1 §3.3 範式): 'binding' 25 件を **root cause 軸** で分割 (SPIR-V binding 衝突 / overlapping binding / sampler binding 等)
5. **Agent 報告検証 step** (η-6 §3.2 範式): stage type / source file 同定の literal grep 検証を patch 化前に必須実施
6. **set/binding allocation 連続帯 reservation** (η-6 §3.1 付随範式): η-7 binding 56-58 使用 → η-8 binding 59+ 連続割当 (= 59-99 帯予約)

### §10.3 B?-η-8 完遂後の想定 cascade exposure 第9層

- main metric non-opaque -1 全達成想定 + 'binding' 25 件解消 = 第9層 emergence shader_cache 305 → 330+ 想定
- 第8層 emergence 残 ('location' 28) は B?-η-9 で sub-bundle 分割対処

### §10.4 第7-8層 emergence 系統整理 sub-bundle 分割推奨表

| sub-bundle | scope | 件数想定 | 主 patch pattern |
|---|---|---|---|
| **B?-η-8** | non-opaque 残 1 (Avatar Eyes 0:570) + 'binding' 25 件 = 計 26 件 主指標 | Agent depth trace runtime preprocess dump + sampler binding 明示 / 衝突解消 | runtime preprocess dump 取得範式確立 |
| **B?-η-9** | 'location' 28 件 | SPIR-V location missing + overlapping location | (要 trace) |

---

## §11 観測点

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 305+ 維持観測点 | 維持中 | B?-η-8 で 330+ 想定、減少時は link 成立路径退行 |
| 2 | cascade pair hypothesis 汎用形 順方向 / 逆方向シフト (§3.5) | 順方向検証成立 (patch 数小規模時) | B?-η-8 以降も継続観測、patch 数規模で順方向 / 逆方向判別 |
| 3 | bvec2/bool → uvec2/uint promote + #define alias 範式 (§3.1) | 適用済 | B?-η-8 以降の std140 不許可 type 出現時の標準 patch pattern |
| 4 | program name collision 確認範式 (§3.2) | 適用済 | B?-η-8 以降の log program 名 不一意検出時の Claude 自力 trace 範式 |
| 5 | 5th-level scope refinement cascade emergence 取込み (§3.3) | 適用済 (phase 2-A) | B?-η-8 でも phase 1 verify 後 cascade emergence 発見時の sub-bundle 内取込 範式継続 (3 条件確認) |
| 6 | nameless block × member collision 範式 (§3.4) | 適用済 (data member 第 4 例) | B?-η-8 で nameless 完全達成、その先は member-level 範式の applicability 監視継続 |
| 7 | Agent capability limit による B?-η-8 移管 (η-6 §3.4) | 適用済 (Avatar Eyes 0:570) | B?-η-8 着手前 trace で runtime preprocess dump 取得手順確立、Agent depth trace 投入 |
| 8 | set/binding allocation 連続割当範式 | binding 14-58 (45 bindings) 使用 | B?-η-8 binding 59+ 連続割当、Vulkan maxDescriptorSetBoundResources 1024 内 |
| 9 | scope refinement 5th-level (η-5 §3.2) 継承適用 (自作 bug 性なし版) | 適用済 (§3.3) | B?-η-8 でも cascade emergence 取込みを 3 条件で判定 |
| 10 | 第7-8層 emergence 残 ('binding' 25 / 'location' 28) sub-bundle 分割 | B?-η-8/9 移管 | sub-bundle 境界 = main metric 達成後の系統別分割 |
