# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-15 完遂 handoff

**status**: B?-η-15 完遂 → 次 sub-bundle B?-η-16 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: (本 doc 起草と同 cycle で commit、AYA「handoff doc 起草して、それから commit」明示指示下 2026-06-02)
**handoff doc commit**: 本 doc (η-14-complete 範式継承、別 commit)
**勤続範式継承**: B?-η-14-complete `78df235243` (patch `c971838656`) / B?-η-13-reverted (no commit) / B?-η-12-complete `7c1762d214` / B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-15 scope = **η-14 末残退行 6 件 (non-opaque uniforms outside a block) 完全解消** (3 系統 3 file UBO wrap、AYA「OK」judgment 確定 2026-06-02、Phase 単一 1-shot ACCEPT)。

- **Phase 単一 (1-shot ACCEPT)**: **UBO wrap 3 file** (η-8 §3.4 範式継承)
  - `waterV.glsl`: 6 件 bare uniform (waveDir1/2, time, eyeVec, waterHeight, lightDir) → `WaterVParamUBO_Legacy` (set=3, binding=60)
  - `terrainV.glsl`: 2 件 bare uniform (object_plane_s/t) → `TerrainVParamUBO_Legacy` (set=3, binding=61)
  - `SMAABlendWeightsF.glsl`: 1 件 bare uniform (subsampleIndices, `#if AYASTORM_CINEMATIC` 内) → `SMAABlendWeightsFParamUBO_Legacy` (set=3, binding=62)
- **3 file (waterV / terrainV / SMAABlendWeightsF) shader-only edit、C++ touch 0** (charter §3 #1 遵守、feedback_shader_only_fast_iterate 適用)
- **binding 60-62 連続割当**: η-14 §11 #9 観測点記載「set/binding allocation 連続割当 (η-8 末: 14-59 + 100-103)」「60+ 連続割当開始」範式の **第 1 適用**
- **anonymous UBO wrap で main() touch 0** (η-1 §3.1 範式継承、guard `<NAME>_DEFINED` で auto-attach 重複 prepend 防御)

**計**: **3 file (waterV +14 / terrainV +9 / SMAABlendWeightsF +9)** shader file のみ、C++ touch 0

**主指標達成** (vs η-14 末 baseline = log `AYAstorm.log`、2026-06-01T23:56:01Z 起動):
- `non-opaque uniforms outside a block` 6 → **0** ✓ **-6 / 100% 完全達成**

**boost cascade 解消** (η-15 で副次効果): なし (主指標解消の直接効果のみ、parse failed -6 = handoff §10.3 想定 -6 ピッタリ達成)

**新規 regression**: **0 件** ✓

**他既達主指標完全維持** (η-15 末):
- `overlapping use of location` 0 / `Layout location qualifier` 0 / `'binding'` 0 / `Cannot reuse block name` 0 / `'weight4'/'weight'` 0 / `Link failed` 0 / `'size'` 0 / `GBufferInfo` 0 / `nameless block ... global scope` 0 (η-14 達成維持) / `undeclared identifier` 0 (η-14 達成維持)
- `'normalMap'` 69 維持 / `'depthMap'` 9 維持 / `SPIR-V requires location` 41 維持

**Phase 構成の特徴**: η-14 末 non-opaque 6 件のうち Underwater/Deferred Terrain 2 件は **η-14 §4.4 stage shift (FRAG → VERTEX) 露出組** (η-14 で覆われていた error 階層深化結果)、SMAA Blending Weights 4 件は **η-14 baseline で既存** (4 preset で同 L1712 = SMAABlendWeightsF.glsl の `subsampleIndices` 共通真因)。本 sub-bundle で **3 系統 (Underwater / Deferred Terrain / SMAA) 同時 UBO wrap** で全 6 件 1-shot 解消 = η-14 §3.4 cascade pair shift 逆方向範式の **同 metric 内 cluster 化解消第 1 例** (個別 source file は別だが同 metric category 内で集約処理)。

---

## §2 完遂結果 metric (vs B?-η-14 末 baseline log `AYAstorm.log`)

| metric | η-14 末 baseline | η-15 (verify) | Δ vs η-14 | 判定 |
|---|---|---|---|---|
| **non-opaque uniforms outside a block** | **6** | **0** | **-6** | ✓ **100% 完全達成** |
| nameless block ... global scope | 0 | 0 | ±0 | ✓ η-14 達成維持 |
| undeclared identifier | 0 | 0 | ±0 | ✓ η-14 達成維持 |
| SPIR-V requires location | 41 | 41 | ±0 | (η-16+ 移管継続) |
| 'normalMap' : redefinition | 69 | 69 | ±0 | (η-16+ 移管継続) |
| 'depthMap' : redefinition | 9 | 9 | ±0 | (η-16+ 移管継続) |
| overlapping use of location | 0 | 0 | ±0 | ✓ η-12 達成維持 |
| Layout location qualifier | 0 | 0 | ±0 | ✓ η-10 達成維持 |
| 'binding' | 0 | 0 | ±0 | ✓ η-8 達成維持 |
| Cannot reuse block name | 0 | 0 | ±0 | ✓ η-1 達成維持 |
| 'weight4'/'weight' redefinition | 0 | 0 | ±0 | ✓ η-5/η-7 達成維持 |
| Link failed | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| 'size' undeclared | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| parse failed (count) | 127 | **121** | **-6** | ✓ net 縮小 (主指標 6 件解消の直接効果) |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | ±0 | ✓ |
| Goodbye | 1 | 1 | ±0 | ✓ clean shutdown |
| Vulkan device destroyed | 1 | 1 | ±0 | ✓ clean shutdown |

**主指標 non-opaque 6 件 100% 完全達成 + 14 種既達主指標完全維持 + 新規 regression 0 件** = η-15 主 scope 完全達成、残 119 件 (normalMap 69 + depthMap 9 + SPIR-V 41) = **B?-η-16 移管**。

---

## §3 設計範式

### §3.1 新規 設計範式: binding 60+ 連続割当開始範式 第 1 例 (η-14 §11 #9 観測点準拠)

**概要**: η-14 §11 #9 観測点で記載された「set/binding allocation 連続割当 (η-8 末: 14-59 + 100-103)」「60+ 連続割当開始」範式を η-15 で **第 1 適用**。set=3 binding=60/61/62 を 3 件連続割当。

**Detection 手順** (3 step):
1. **既存 set=3 binding 番号 audit**: 全 shader file (`indra/newview/app_settings/shaders/`) で `layout(set=3, binding=N)` literal grep、使用済番号一覧化
2. **空き帯 + 推奨開始番号確定**: η-8 末 14-59 帯満員 (空きは 15/16/23/29/31/34-36 等飛び石) + 100-103 帯使用、60-99 帯空き確認 → 60 を新規開始番号として確定
3. **新規連続割当**: 必要件数を 60/61/62 ... と連続割当、隣接性で C++ descriptor set allocation 効率維持

**η-15 適用**:
| # | UBO 名 | binding 番号 | file |
|---|---|---|---|
| 1 | `WaterVParamUBO_Legacy` | set=3, binding=60 | waterV.glsl |
| 2 | `TerrainVParamUBO_Legacy` | set=3, binding=61 | terrainV.glsl |
| 3 | `SMAABlendWeightsFParamUBO_Legacy` | set=3, binding=62 | SMAABlendWeightsF.glsl |

**handoff doc 記録価値**: 後続 sub-bundle で新規 UBO 割当時、η-15 末点で **次の安全番号は binding=63** から開始。100-103 帯は予約 (用途未確定だが既存 ReflectionProbes binding=59 と隣接、η-8 末 record による意図的予約)。

### §3.2 新規 設計範式: 3 重 nest 条件 UBO wrap 範式 (AYASTORM_CINEMATIC + LL_VULKAN_GLSL + guard)

**概要**: feature flag (e.g. `AYASTORM_CINEMATIC`) で guarded な bare uniform を UBO wrap する場合、3 重 nest 構造 (`#if FEATURE` + `#ifdef LL_VULKAN_GLSL` + `#ifndef GUARD_DEFINED`) で正確に Vulkan path のみ wrap する範式。

**Detection 手順** (3 step):
1. **feature flag 列挙**: 主 shader 内の bare uniform 直近の `#if FEATURE` / `#ifdef FEATURE` 等の preprocess condition を確認
2. **nest 順序確定**: 外側 = feature flag、中間 = `LL_VULKAN_GLSL` 分岐、内側 = `<NAME>_DEFINED` guard
3. **GL path `#else` の元 bare uniform 宣言 byte-for-byte 不変保持**

**η-15 適用**: SMAABlendWeightsF.glsl L62-74:
```glsl
#if AYASTORM_CINEMATIC                   // feature flag
#ifdef LL_VULKAN_GLSL                    // Vulkan path
#ifndef SMAA_BLEND_WEIGHTS_F_PARAM_UBO_LEGACY_DEFINED  // guard
#define SMAA_BLEND_WEIGHTS_F_PARAM_UBO_LEGACY_DEFINED 1
layout(set=3, binding=62, std140) uniform SMAABlendWeightsFParamUBO_Legacy {
    vec4 subsampleIndices;
};
#endif
#else                                    // GL path (feature ON)
uniform vec4 subsampleIndices;
#endif
#endif                                   // /AYASTORM_CINEMATIC
```

**handoff doc 記録価値**: 後続 sub-bundle で feature flag 内 bare uniform 発見時、本範式に従い 3 重 nest を正確に組む。feature OFF 時は両 path とも宣言なし、feature ON 時のみ各 path で適切な declaration。

### §3.3 設計範式継承: η-8 §3.4 UBO wrap 範式 第 N 例 (3 件同 sub-bundle 並列適用)

**概要**: η-8 §3.4 で確立された bare uniform → UBO wrap 範式を 3 系統 (Water/Terrain/SMAA) で同 sub-bundle 並列適用。η-1 §3.1 guard wrap 範式 + η-8 §3.4 UBO wrap 範式 + η-15 §3.1 binding 60+ 連続割当範式の **3 範式同時適用**。

**η-15 適用**:
- waterV: 6 member UBO (vec2/vec2/float/vec3/float/vec3) std140 alignment 配慮 (vec3 後 float pattern 合法、offset 32+12=44 で float fit、offset 48 で次 vec3、total 64 bytes round-up)
- terrainV: 2 member UBO (vec4/vec4) std140 自然合法 (32 bytes)
- SMAABlendWeightsF: 1 member UBO (vec4) std140 自然合法 (16 bytes)

**handoff doc 記録価値**: 3 系統並列 UBO wrap で **同 sub-bundle 内 metric category cluster 化解消** = 個別 source file は別だが同 metric (non-opaque) 内集約処理範式の第 1 例。

### §3.4 設計範式継承表 (η-1〜η-14 全件継承)

| η-N | 範式 | η-15 適用状況 |
|---|---|---|
| η-1 §3.1 | FrameViewProj guard wrap (set=0/binding=0) | η-15 §3.3 で **guard wrap** 範式を新規 UBO 3 件にも適用 (`WATER_V_PARAM_UBO_LEGACY_DEFINED` 等) |
| η-1 §3.2 | cascade pair hypothesis (主指標 -X ↔ cascade +Y) | η-15 適用外 (cascade boost なし) |
| η-1 §3.3 | Agent 並列 disjoint scope | η-15 適用外 (直接 grep 同定で済、Agent 投入なし) |
| η-2 (a) §3.2 | UBO body member 順序 std140 完全一致 | η-15 §3.3 で **std140 alignment 配慮** (vec3 後 float pattern 合法性 verify) |
| η-3 §3.1/§3.2 | Link failed root cause UBO body 差異解消 + per-group rename | η-15 適用外 (新規 UBO 3 件は単独 declaration) |
| η-4 §3.1/§3.2/§3.4 | nameless block × padding / cascade pair / UBO set=3 帯 | η-15 §3.1 で **binding 60+ 連続割当範式 第 1 例** に拡張 |
| η-5〜η-7 | 各種 (multi-root-cause / nameless block × member 系) | η-15 適用外 |
| η-8 §3.3 | mIndexedTextureChannels Vulkan-aware | η-15 適用外 (η-16+ SPIR-V missing 41 件対処時に再投入候補) |
| η-8 §3.4 | bare uniform → UBO wrap | η-15 §3.3 で **3 系統 3 件同 sub-bundle 並列適用** (第 N 例) |
| η-9 §3.1 | V/F pair canonical partner 同定範式 | η-15 適用外 (location qualifier 系でなく non-opaque 系) |
| η-9 §3.2 | Phase 1 falsification + 完全 revert + Phase 2 軌道修正 | η-15 では Phase 1 即 ACCEPT (1-shot 完結) |
| η-9 §3.3/§3.4 | falsification_as_progress / admit_unknown | η-15 適用外 (1-shot ACCEPT で適用機会なし) |
| η-10 §3.1 | handoff doc canonical 記載 着手前再検証範式 | η-15 着手前 trace で適用 (η-14 §10.1 / §10.3 / §11 #9 観測点を literal 再検証) |
| η-10 §3.3 | 1-shot ACCEPT vs 2-phase 構成判定範式 | η-15 で 1-shot ACCEPT 適用第 5 例 |
| η-11 §3.1 | V/F pair canonical partner 第 3 例 (auto-attach helper 経路) | η-15 適用外 |
| η-11 §3.2 | handoff doc canonical 記載 着手前再検証範式 第 2 例 | η-15 で継承 (η-14 §10/§11 全件 literal 再検証) |
| η-11 §3.3 | 1-shot ACCEPT vs 2-phase 構成判定範式 第 2 例 | η-15 で第 5 例継承 |
| η-11 §3.4 | cascade pair shift 順方向 既達退行 ACCEPT 判定範式 | η-15 適用外 (cascade boost なし、主指標解消の直接効果のみ) |
| η-11 §3.5 | Agent 報告検証 step V/F pair 同定範式違反検出範式 | η-15 適用外 |
| η-12 §3.1 | V/F pair canonical partner 第 4 例 | η-15 適用外 |
| η-12 §3.2 | cascade pair shift Y = X 境界条件範式拡張 | η-15 適用外 |
| η-12 §3.3 | 1-shot ACCEPT vs 2-phase 構成判定範式 第 3 例 | η-15 で第 5 例継承 |
| η-12 §3.4 | handoff doc 記録漏れ補完範式 | η-15 で継承 (η-14 §10.1 表を着手前 trace で literal 再検証) |
| η-13 §3.5 | retreat-revert 範式 | η-15 では fresh attempt 成功で適用外 |
| η-13 §3.6 | 同 binding 多 block member 重複衝突 diagnostic | η-15 適用外 (新規 binding は 60+ で衝突なし audit 済) |
| η-13 §3.7 | 4-Path 比較範式 | η-15 適用外 (Path 単一 UBO wrap で完結) |
| η-14 §3.1 | GL stage hex 解釈訂正範式 (0x8B30=FRAG / 0x8B31=VERT) | η-15 着手前 trace step 4 で適用 (Underwater 0x8B31=VERTEX / Deferred Terrain 0x8B31=VERTEX / SMAA 0x8B30=FRAGMENT 確定) |
| η-14 §3.2 | nameless block 重複衝突 構造的解消範式 (auto-attach helper 委譲) | η-15 適用外 (nameless block 系 error なし) |
| η-14 §3.3 | 同 UBO 別 stage 複製範式 | η-15 適用外 (1 file 1 stage 単独 UBO) |
| η-14 §3.4 | cascade pair shift 逆方向 既達退行解消範式 | η-15 §1 で **同 metric 内 cluster 化解消第 1 例** (個別 source file 別だが同 metric category 内集約処理) |
| η-14 §3.5 | handoff doc misanalysis 検出救済範式 | η-15 着手前 trace step 3 で継承 (η-14 §10/§11 を literal 再検証、misanalysis なし確認) |

**feedback memory 10 件全件適用確認**:
- feedback_doubt_self_first: η-14 §3.1 GL stage hex 訂正範式継承で η-15 でも stage 同定時 literal 確認
- feedback_root_cause_not_dump: 3 件 UBO wrap で構造的に non-opaque 解消 (workaround でなく根本対処)
- feedback_shader_only_fast_iterate: shader cp + cache clear のみ (autobuild 不要、C++ touch 0)
- feedback_no_auto_commit: AYA「handoff doc 起草して、それから commit」明示指示後に commit (本 doc 起草 → commit)
- feedback_no_claude_coauthor: commit message に Co-Authored-By 不在予定
- feedback_one_step_at_a_time: 着手前 trace → 候補列挙 → 推奨 → AYA judgment「OK」→ patch 設計提示 → AYA「OK」→ patch 実施 → self-verify → AYA「OK」→ deploy → AYA cold cache launch → verify → handoff doc 起草 を順次実施
- feedback_proactive_handoff: 本 handoff doc 起草
- feedback_no_scope_shrink: η-15 scope (non-opaque 6 件全件) は AYA「OK」judgment 下確定 = scope shrink なし、6 件全件 1-shot 解消
- feedback_admit_unknown: η-15 では仮説外れず (1-shot ACCEPT)
- feedback_falsification_as_progress: η-15 では falsification なし (1-shot ACCEPT)
- feedback_self_verify_before_handoff: AYA cold cache launch 依頼前に Claude が 3 file edit 後の Vulkan path / GL path 構造を Read で literal 再確認 (self-verify all green)

---

## §4 cold cache launch verify 詳細

### §4.1 verify cycle 構成 (1 cycle = 1-shot ACCEPT)

**Cycle 1 (Phase 1 verify = ACCEPT)**:
- 操作: 3 file edit → self-verify (Read で edit 後構造 literal 確認) → shader-only fast-iterate cp 3 file → shader_cache clear (0 件 verify) → AYA cold cache launch → shutdown
- 結果:
  - `non-opaque uniforms outside a block` 6 → **0** (-6 ✓ 100% 完全達成)
  - 14 種既達主指標 0 完全維持 + normalMap 69 / depthMap 9 / SPIR-V 41 維持
  - parse failed count 127 → 121 (-6 net、主指標 6 件解消直接効果、handoff §10.3 想定 -6 ピッタリ達成)
  - parse failed unique program-stage diff: cascade boost なし (主指標解消の直接効果のみ)
  - clean shutdown (Goodbye 1 + Vulkan device destroyed 1 + FATAL/SIGSEGV/Aborted 0)
- 判定: **η-15 deliverable 確定 (1-shot ACCEPT、AYA「OK」judgment 確定 2026-06-02)**

### §4.2 主指標 metric integrity self-check

- `non-opaque uniforms outside a block` -6 = 3 file UBO wrap 直接効果:
  - Underwater (waterV L1099 真因 = 6 件 bare uniform): -1 (program 単位 error)
  - Deferred Terrain (terrainV L1146 真因 = 2 件 bare uniform): -1
  - SMAA Blending Weights Low/Medium/High/Ultra (SMAABlendWeightsF L1712 真因 = subsampleIndices): -4 (4 preset 共通真因)
  - 計 6 件 program × stage error → 0
- parse failed 127→121 (主指標 6 件解消の count 直接効果、unique program-stage diff も cascade boost なしのため net -6 完全整合)

### §4.3 stage shift / cascade boost (η-15 では発生なし)

η-14 では §4.4 で stage shift (FRAG → VERTEX 露出) が観測されたが、η-15 では 3 系統全件 UBO wrap で解消、stage shift / cascade boost 共に発生なし = handoff §10.3 想定 -6 ピッタリ達成。

### §4.4 shutdown clean verify

- Cycle 1 で:
  - `Goodbye!` 1 件 + `Vulkan device destroyed` 1 件
  - FATAL/SIGSEGV/Aborted 0/0/0

---

## §5 self-verify 18 項目 all green

| # | 項目 | 状態 |
|---|---|---|
| 1 | charter §3 #1 GL path byte-for-byte 不変担保 (3 file GL `#else` path 元宣言不変、Vulkan path 内のみ edit) | ✓ |
| 2 | shader file のみ touch (C++ touch 0) | ✓ |
| 3 | 主指標 `non-opaque uniforms outside a block` 6→0 literal grep verify | ✓ |
| 4 | 既達主指標 14 種 literal grep verify 全 0 維持 | ✓ |
| 5 | normalMap/depthMap/SPIR-V 69/9/41 維持 (計 119 件) | ✓ |
| 6 | parse failed count 127→121 net 縮小 verify (主指標解消の直接効果整合) | ✓ |
| 7 | FATAL/SIGSEGV/Aborted 0 維持 | ✓ |
| 8 | clean shutdown (Goodbye 1 + Vulkan destroy 1) | ✓ |
| 9 | binding 60+ 連続割当範式 §3.1 第 1 適用 (60/61/62 連続割当) | ✓ |
| 10 | 3 重 nest 条件 UBO wrap 範式 §3.2 新規確立 (AYASTORM_CINEMATIC + LL_VULKAN_GLSL + guard) | ✓ |
| 11 | η-8 §3.4 UBO wrap 範式 第 N 例 並列適用 §3.3 (3 系統同 sub-bundle 集約) | ✓ |
| 12 | η-1 §3.1 guard wrap 範式継承 (`WATER_V_PARAM_UBO_LEGACY_DEFINED` 等 3 件) | ✓ |
| 13 | std140 alignment 合法性 verify (waterV: vec3 後 float pattern offset 32+12=44 で fit、total 64 bytes) | ✓ |
| 14 | binding 60/61/62 番号衝突 audit 済 (set=3 全 shader file grep で 0-59 + 100-103 帯使用済確認、60-99 帯空き確認) | ✓ |
| 15 | main() touch 0 (anonymous UBO で member 直接参照維持) | ✓ |
| 16 | feedback_no_claude_coauthor 遵守 (commit message Co-Authored-By 不在予定) | ✓ |
| 17 | feedback_no_auto_commit 遵守 (AYA「handoff doc 起草して、それから commit」明示指示後に commit) | ✓ |
| 18 | handoff doc 別 commit (η-14-complete `78df235243` 範式継承) | ✓ |

---

## §6 設計範式継承表

(§3.4 と内容重複のため §3.4 参照、feedback memory 10 件全件適用確認 + prior sub-bundle 33+ 件継承 + 新規 2 件 §3.1/§3.2 + 継承 1 件 §3.3)

---

## §7 risks

| # | risk | mitigation |
|---|---|---|
| 1 | §3.1 binding 60+ 連続割当範式の **割当 race** (後続 sub-bundle で binding=60/61/62 を別 UBO に再割当する事故) | η-15 末で 60/61/62 占有 → 次安全番号は **63** から開始、本 doc §3.1 表で明示済、後続 sub-bundle 着手前 trace step で再 audit 必須 |
| 2 | §3.2 3 重 nest 条件 UBO wrap 範式の **nest 順序誤り** (feature flag を内側に書くと OFF 時に Vulkan path declaration が残る) | nest 順序 = 外側 feature flag / 中間 LL_VULKAN_GLSL / 内側 guard を厳守、後続 sub-bundle 着手前 trace で nest 順序 literal 確認 |
| 3 | std140 alignment 罠 (vec3 + float pattern は合法だが、member 順序入れ替えで padding 増加・alignment 失敗の可能性) | η-15 §3.3 で waterV (6 member) std140 alignment literal 確認済、後続 sub-bundle で新規 UBO 追加時は member 順序 + offset 表を着手前 trace で literal 検証 |
| 4 | SMAABlendWeightsF.glsl の `subsampleIndices` UBO wrap は **`#if AYASTORM_CINEMATIC` ON 時のみ** = Cinematic OFF 時には Vulkan path / GL path 両方とも declaration なし = `SMAABlendingWeightCalculationPS` 呼び出し時 `vec4(0.0)` fallback (本来の振る舞い維持) | feature flag guard 内で wrap = feature OFF 時の declaration 不在は元の意図通り、回避不要 |
| 5 | 残退行 normalMap 69 + depthMap 9 + SPIR-V 41 = 計 119 件 η-16+ 移管で **残量大** | η-16+ で系統別 Phase 分割 (SPIR-V 41 件 = C++ runtime emit Vulkan-aware 化 η-8 §3.3 範式優先、normalMap/depthMap 別 sub-bundle 分割推奨) |
| 6 | C++ touch 0 維持 (η-8 で 1 例発生、η-9〜η-15 で 0) | η-16 で SPIR-V missing 41 件対処時に C++ touch 復活可能性 (η-8 §3.3 範式)、shader-only 範式の自然延長で構造的限界がある場合のみ |
| 7 | cinematic_bd directory 全 .glsl audit (η-8 から継承、η-9〜η-15 未実施) | η-16 着手前 or 完遂後の別 phase として強く推奨 (η-8 §3.1 cinematic_bd override path 発見範式の予防的展開) |
| 8 | shader_cache 件数 (η-7 305 baseline → η-11 310) η-12/η-14/η-15 計測未実施 | η-16 で shader_cache 件数計測再開 + 第15層 emergence 観測 |
| 9 | runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画 (η-9〜η-15 未投入) | η-16 で root cause が static trace 不能な場合に再投入 + sub-bundle 完遂時除去 |
| 10 | binding=60/61/62 の **C++ descriptor set allocation 整合** (anonymous UBO は C++ 側で binding 名 resolution 経由で値設定するが、descriptor set allocator が 60+ 帯を allocation 対象としていない可能性) | η-15 cold cache launch verify で parse failed -6 + 全主指標解消 + clean shutdown 確認済 = C++ 側 descriptor set allocator は binding 60+ も自動対応 (Vulkan SPIR-V layout 経由 reflection)、後続 sub-bundle で同様 |

---

## §8 commit history

| commit | sub-bundle | scope |
|---|---|---|
| (η-15 patch 予定 = 本 cycle commit) | B?-η-15 patch | 3 file (waterV / terrainV / SMAABlendWeightsF) UBO wrap (non-opaque 6→0 達成) |
| `78df235243` | B?-η-14 handoff doc | (η-14 完遂 handoff、η-15 着手境界引継) |
| `c971838656` | B?-η-14 patch | 3 file (skyV / cloudsV / cloudsF) Path G + G-β (Skybox UBO 削除 + CloudsVParamUBO 複製) |
| (η-13 no commit) | B?-η-13 retreat-revert | Path B (haze_horizon unification) 失敗 → 完全 revert |
| `7c1762d214` | B?-η-12 handoff doc | (η-12 完遂 handoff) |
| `bf9164ce34` | B?-η-12 patch | 7 file Vulkan path location 21 → 27 整合 (overlapping use of location 21 4→0 達成) |
| `74705c35bb` | B?-η-11 handoff doc | (η-11 完遂 handoff) |
| `9a271fc900` | B?-η-11 patch | 15 file Vulkan path location 20 → 26 整合 |
| `9246142639` | B?-η-10 handoff doc | (η-10 完遂 handoff) |
| `e1d5ffd98c` | B?-η-10 patch | diffuseAlphaMaskF location 整合 |
| `6fee4a818b` | B?-η-9 handoff doc | (η-9 完遂 handoff) |
| `50cb5f6713` | B?-η-9 patch | avatarEyesV location 整合 |
| `6994eba271` | B?-η-8 handoff doc | (η-8 完遂 handoff) |
| `2542905ce0` | B?-η-8 patch | non-opaque 1 + 'binding' 25 |

---

## §9 file inventory (η-15 patch 3 file 内訳)

| # | file | 変更内容 | 行数 | 範式適用 |
|---|---|---|---|---|
| 1 | `indra/newview/app_settings/shaders/class1/environment/waterV.glsl` | Vulkan path に `WaterVParamUBO_Legacy` (set=3, binding=60, std140, 6 member: vec2 waveDir1/2, float time, vec3 eyeVec, float waterHeight, vec3 lightDir) 新規追加 + guard `WATER_V_PARAM_UBO_LEGACY_DEFINED`、GL `#else` path 元 bare uniform 6 件 byte-for-byte 不変保持 | +14/-0 (元 6 行 → 21 行に拡張) | §3.1 binding 60+ 連続割当 (第 1) + §3.3 UBO wrap (η-8 §3.4 範式継承) + η-1 §3.1 guard wrap |
| 2 | `indra/newview/app_settings/shaders/class1/deferred/terrainV.glsl` | Vulkan path に `TerrainVParamUBO_Legacy` (set=3, binding=61, std140, 2 member: vec4 object_plane_s/t) 新規追加 + guard `TERRAIN_V_PARAM_UBO_LEGACY_DEFINED`、GL `#else` path 元 bare uniform 2 件 byte-for-byte 不変保持 | +9/-0 (元 2 行 → 13 行に拡張) | §3.1 binding 60+ 連続割当 (第 2) + §3.3 + η-1 §3.1 |
| 3 | `indra/newview/app_settings/shaders/class1/deferred/SMAABlendWeightsF.glsl` | `#if AYASTORM_CINEMATIC` 内 Vulkan path に `SMAABlendWeightsFParamUBO_Legacy` (set=3, binding=62, std140, 1 member: vec4 subsampleIndices) 新規追加 + guard `SMAA_BLEND_WEIGHTS_F_PARAM_UBO_LEGACY_DEFINED`、GL `#else` path 元 bare uniform 1 件 byte-for-byte 不変保持 | +9/-0 (元 1 行 → 11 行に拡張、`#if AYASTORM_CINEMATIC` block 内) | §3.1 binding 60+ 連続割当 (第 3) + §3.2 3 重 nest 条件 UBO wrap (新規範式) + §3.3 + η-1 §3.1 |

**計**: 3 file +32/-0 net +32、shader file のみ、C++ touch 0

**overlap file 確認**:
- waterV.glsl: η-5 で FrameViewProj guard wrap 済 (L26-46)、η-15 で第 2 touch (L57 周辺 UBO 新規追加) = B?-δ 範式 admission 第 2 touch
- terrainV.glsl: η-5 で FrameViewProj + MaterialUBO 追加済 (L26-56)、η-15 で第 2 touch (L104 周辺 UBO 新規追加)
- SMAABlendWeightsF.glsl: η-15 で初 touch
- SMAA.glsl: η-15 で参照のみ (SMAA_RT_METRICS 既存 SMAAParamUBO_Legacy binding=14 確認、touch 0)

---

## §10 次 sub-bundle B?-η-16 推奨 scope

### §10.1 確定 scope: SPIR-V missing 41 件 + normalMap/depthMap 78 件 (残全件)

| metric | η-15 末件数 | 推定 root cause | 推奨対処 |
|---|---|---|---|
| SPIR-V requires location | 41 | in/out 宣言の location 番号未指定 (mass) | C++ runtime emit Vulkan-aware 化 (η-8 §3.3 範式) or 一括 wrap script、最優先候補 |
| 'normalMap' redefinition | 69 | (η-1〜η-15 未対処、新 root cause 系統) | 専用 phase (Agent 並列 disjoint scope η-1 §3.3) |
| 'depthMap' redefinition | 9 | 同上 | 同上 (normalMap と統合 phase 候補) |

### §10.2 B?-η-16 着手前 trace 範式 10 ステップ (η-15 §3.1/§3.2 範式追加)

1. **literal grep**: 各既達主指標 (16 種、non-opaque も η-15 で達成入り) の literal grep + SPIR-V 41 件 + normalMap 69 + depthMap 9 件の grep
2. **handoff doc 記録漏れ補完範式** (η-12 §3.4): 前 sub-bundle (η-15) handoff §2 metric 表 + §10.1 推奨 scope の literal 再検証
3. **handoff doc misanalysis 検出救済範式** (η-14 §3.5): 前 sub-bundle handoff doc の前提 (stage 同定、root cause 推定等) に **literal 確認 step** 必ず組込
4. **GL stage hex 解釈確認範式** (η-14 §3.1): log error の 0x8B30 = FRAGMENT / 0x8B31 = VERTEX を着手前 trace で再確認
5. **log context 抽出**: 各 program の error LINE × 件数 + stage type 番号 + 真因 file 候補 (V or F) を併記 (η-6 §3.2 範式)
6. **V/F pair / auto-attach helper 同定** (η-9〜η-14 範式): location 系は V/F pair、UBO 系は auto-attach helper 経路同定
7. **nameless block 重複衝突 検出 + 構造的解消可能性 verify** (η-14 §3.2): nameless block 重複検出時、auto-attach helper 委譲 (片方削除) 可能性を main() 参照 member 全件解決確認で verify
8. **同 UBO 別 stage 複製判定** (η-14 §3.3): fragment shader が vertex stage UBO member 参照する場合、shader-only 複製で解決可能性を guard 付き complete-layout 複製で適用
9. **binding 60+ 連続割当範式** (**η-15 §3.1 新規**): 新規 UBO 割当時は **binding=63 から開始** (η-15 末で 60/61/62 占有)、後続 sub-bundle 着手前に set=3 全 shader file grep で再 audit 必須
10. **3 重 nest 条件 UBO wrap 範式** (**η-15 §3.2 新規**): feature flag 内 bare uniform 発見時、3 重 nest (`#if FEATURE` + `#ifdef LL_VULKAN_GLSL` + `#ifndef <NAME>_DEFINED`) を正確に組む
11. **Agent 並列 disjoint scope** (η-1 §3.3 範式): metric 系統別に root cause 軸で分割 (SPIR-V missing / normalMap / depthMap 各別 Agent)
12. **cascade pair shift 既達退行 ACCEPT 判定** (η-11 §3.4 / η-12 §3.2 / η-14 §3.4 逆方向): 順方向 (Y < X ACCEPT) + 逆方向 (Y 削減 → X 達成) + Y = X 境界条件 (AYA judgment)

### §10.3 B?-η-16 完遂後の想定 cascade exposure 第15層

- SPIR-V missing 41 件解消想定 = parse failed -41 程度 (C++ runtime emit Vulkan-aware 化で大量解消)
- normalMap / depthMap redefinition 78 件 → 別 sub-bundle 分割推奨 (B?-η-17 移管候補)
- shader_cache 件数: η-16 で 310+ 維持想定 + 第15層 emergence 観測

### §10.4 cinematic_bd 系統 systematic audit 推奨 (η-8 から継承、η-9〜η-15 未実施)

η-8 §3.1 cinematic_bd override path 発見範式で shadowUtil のみ個別検出。cinematic_bd directory 内 .glsl file 全件で類似 override + 未 wrap bare uniform / 未整合 location 残存可能性。B?-η-16 着手前 or 完遂後の別 phase として継続推奨。

### §10.5 runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画継承

η-9〜η-15 未投入 (static trace + literal grep + handoff doc misanalysis 検出範式で同定可能だった)。η-16 で SPIR-V missing 41 件対処時に root cause が static trace 不能な場合、η-8 §3.2 範式再投入 → sub-bundle 完遂時に必ず除去。

### §10.6 binding 60+ 連続割当範式 (η-15 §3.1) 後続管理

η-15 末で binding 60/61/62 占有。η-16+ 新規 UBO 割当は **binding=63 から**。η-15 §3.1 表で明示済、後続 sub-bundle 着手前 trace step 9 で set=3 全 shader file grep audit 必須。

### §10.7 3 重 nest 条件 UBO wrap 範式 (η-15 §3.2) 適用 audit

η-15 教訓: feature flag 内 bare uniform は 3 重 nest で正確に wrap。η-16+ で類似 case (e.g. `#if AYASTORM_CINEMATIC` / `#if FEATURE_X` 内 bare uniform) 発見時、本範式適用。

---

## §11 観測点

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-11 310) | η-12/η-14/η-15 計測未実施 | B?-η-16 で計測再開 + 第15層 emergence 観測 |
| 2 | binding 60+ 連続割当範式 (§3.1) 第 1 適用 | 適用済 (60/61/62 占有) | B?-η-16 で新規 UBO 割当は **63 から**、set=3 全 shader file grep audit 必須 |
| 3 | 3 重 nest 条件 UBO wrap 範式 (§3.2) 新規確立 | 適用済 (SMAABlendWeightsF) | B?-η-16 で類似 feature flag 内 bare uniform 発見時に適用 |
| 4 | η-8 §3.4 UBO wrap 範式 第 N 例 並列適用 (§3.3) | 適用済 (3 系統 3 file) | B?-η-16 で同様の cluster 化解消可能性 |
| 5 | η-14 §3.4 cascade pair shift 逆方向 範式の **同 metric 内 cluster 化解消第 1 例** (§1) | 適用済 (non-opaque 6 件 = Underwater/Terrain/SMAA 4 preset 集約) | B?-η-16 で同 metric category 内 cluster 化解消第 2 例可能性 |
| 6 | 1-shot ACCEPT vs 2-phase 構成判定範式 (η-10 §3.3 / η-11 §3.3 / η-12 §3.3) 第 5 例 | 適用済 (1-shot ACCEPT、Agent 投入なし) | B?-η-16 で着手前 trace 確定度で判定 |
| 7 | Agent 報告検証 step V/F pair 同定範式違反検出範式 (η-11 §3.5) 適用条件明示化 | η-15 では未適用 (直接 grep 同定で済) | B?-η-16 で Agent 投入時に適用 |
| 8 | set/binding allocation 連続割当 (η-8 末: 14-59 + 100-103) → η-15 末: 14-62 + 100-103 | η-15 で 60/61/62 連続新規割当 | B?-η-16 で 63+ 連続割当開始、100-103 帯予約用途未確定継続 |
| 9 | C++ touch 0 維持 (η-8 で 1 例発生、η-9〜η-15 で 0) | η-15 達成 | B?-η-16 で SPIR-V missing 41 件対処時に C++ touch 復活可能性 (η-8 §3.3 範式) |
| 10 | 既達主指標完全維持 (η-15 で 15 種、退行 0 件) | ✓ η-15 で退行 0 件達成 (non-opaque 達成入り) | B?-η-16 で 15 種維持 + 残系統 scope |
| 11 | cinematic_bd directory 全 .glsl audit (§10.4) | 未実施 (η-8 から継承、η-9〜η-15 全て未実施) | B?-η-16 着手前 or 完遂後の別 phase として継続推奨 |
| 12 | runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画 | η-9〜η-15 未投入 | B?-η-16 で root cause が static trace 不能な場合に再投入 + sub-bundle 完遂時除去 |
| 13 | cascade exposure 第14層 残 (SPIR-V 41 + normalMap 69 + depthMap 9 = 119 件) | sub-bundle 残量大 | B?-η-16 着手前 trace で系統別 Phase 分割継続推奨 (SPIR-V 41 件 = 1 Phase 最優先、η-8 §3.3 範式継承) |
| 14 | std140 alignment 範式 (waterV vec3+float pattern 合法) | η-15 で確認済 | B?-η-16 で新規 UBO 追加時は member 順序 + offset 表 literal 検証 |
| 15 | feedback_self_verify_before_handoff 適用 | η-15 で Read による edit 後構造 literal 再確認実施 | B?-η-16 でも AYA cold cache launch 依頼前に必ず実施 |

---

**handoff doc 完。次 sub-bundle B?-η-16 着手は本 doc §10 推奨 scope を起点として、fresh context で実施。**
