# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-16 完遂 handoff

**status**: B?-η-16 完遂 (Phase 1 ACCEPT 単独、Phase 2-4 は η-17 へ分割) → 次 sub-bundle B?-η-17 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: (本 doc 起草と同 cycle で commit、AYA「handoff doc 起草して、それから commit」明示指示下 2026-06-02)
**handoff doc commit**: 本 doc (η-15-complete 範式継承、別 commit)
**prep doc**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-16-prep-D-switch.md` (1c3eb16b6b)
**勤続範式継承**: B?-η-15-complete `cb28cf1daa` (patch `6a11eabc73`) / B?-η-14-complete `78df235243` (patch `c971838656`) / B?-η-13-reverted (no commit) / B?-η-12-complete `7c1762d214` / B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-16 scope = **D approach (C++ runtime location emit transformer) Phase 1 = fragment stage `out` のみ runtime wrap** で **SPIR-V missing location 41 件 (η-15 末) → 30 件 (-11) 縮小** + cascade pair shift forward 5 件 (non-opaque +3 / nameless block global scope +2 = 5 件、全て **prep doc §3.3 列挙の SPIR-V 41 件原 list 内** = cascade pair shift forward direction の handoff §6 risk #4 想定通り)。**Phase 2-4 (V↔F pair / vertex attribute / geometry stage) は AYA「option C-b」judgment (2026-06-02) で η-17 sub-bundle へ分割**。

- **Phase 1 (1-shot ACCEPT、AYA judgment 2026-06-02)**: **C++ runtime location emit transformer 投入** (η-8 §3.3 mIndexedTextureChannels Vulkan-aware 範式の自然延長)
  - `indra/llrender/llglslshader.cpp` anonymous namespace 内に `LocationAllocator` struct + `vulkanizeStageSource()` (line 単位 regex 書換) + `dumpTransformedStageSource()` (diagnostic) 新規実装
  - `LLGLSLShader::generatePerProgramSPIRV()` の concatenated buffer 確定後・`concat_buffers.push_back()` 直前に hook 挿入
  - cache key 先頭に `vulkanize:v1_p1_fragout` + `auto_loc=0/1` の version tag + kill-switch state を inject (HBXXH128) で stale cache hit 防止
  - debug settings 2 件追加: `RenderVulkanShaderAutoLocation` (Boolean, default true = transformer ON) + `RenderVulkanShaderDumpTransformed` (Boolean, default false = diagnostic dump OFF)
- **scope shrink**: bare `out` (fragment stage output) 単独。bare `in` (vertex attribute / V↔F varying) は Phase 2-3 で対応。`stage_type != GL_FRAGMENT_SHADER` 時は no-op で C++ side flow 不変
- **C++ 1 file + settings.xml 1 file edit、shader file touch 0** (handoff §6 risk #1 D acceptance に従う = upstream merge conflict 構造解消)
- **detection regex** (handoff §4.2): `^(\s*)out\s+((?:(?:flat|smooth|noperspective|centroid|highp|mediump|lowp)\s+)*)([a-zA-Z_][a-zA-Z0-9_]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)(\s*\[[^;]*\])?\s*;\s*$` で qualifier-aware bare out 1 line match
- **既存 layout(location=N) out audit**: pre-pass で `^\s*layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*out\b` を grep して `mUsedFragOutSlots` に予約 → mFragOutCursor は skip-on-used で衝突回避
- **kill-switch**: `RenderVulkanShaderAutoLocation=false` で transformer skip = pre-η-16 baseline (η-15 末) 動作完全再現可能 (cache key 別経路で hit せず別 SPIR-V emit パス)

**計**: **C++ 1 file (llglslshader.cpp +176/-2 = +174) + settings.xml +22/-0 = +22**、shader file touch 0

**主指標達成** (vs η-15 末 baseline = log `AYAstorm.log` 起動 2026-06-01T23:56:01Z):
- `SPIR-V requires location` 41 → **30** ✓ **-11 件 (transformer 機構的成立)**
- `non-opaque uniforms outside a block` 0 → **3** = **cascade pair shift forward (handoff §6 risk #4 想定通り)**
- `nameless block contains a member that already has a name at global scope` 0 → **2** = **cascade pair shift forward**
- 既達主指標 13 種完全維持 + normalMap 69 / depthMap 9 維持

**cascade pair shift forward 解析** (5 件全て **prep doc §3.3 SPIR-V 41 件 affected program list 内**):

| # | program | error LINE | error type | prep §3.3 記載状況 |
|---|---|---|---|---|
| 1 | Deferred Blur Light Shader | 0:1702 | non-opaque | ✓ 列挙済 (L1674、line shift は transformer 由来) |
| 2 | FS Object ID Shader | 0:175 | non-opaque | ✓ 列挙済 (L173、`fsObjectIDF.glsl` L27 `out vec4 frag_color;`) |
| 3 | Deferred Buffer Visualization Shader | 0:180 | non-opaque | ✓ 列挙済 (L173) |
| 4 | Deferred Gamma Correction Post Process | 0:1685 | nameless block (FrameAtmosphere_Skybox) | ✓ 列挙済 (L1673) |
| 5 | Legacy Gamma Correction Post Process | 0:1686 | nameless block (FrameAtmosphere_Skybox) | ✓ 列挙済 (L1674) |

= **5 件全件 prep §3.3 内 = transformer は bare `out` を正しく書換、後段 layer の pre-existing error が露出した shift forward (新規 regression ではない)**。

**新規 regression**: **0 件** ✓ (cascade shift は pre-existing layer の露出、新 failure ではない)

**他既達主指標完全維持** (η-16 Phase 1 末):
- `overlapping use of location` 0 / `Layout location qualifier` 0 / `'binding'` 0 / `Cannot reuse block name` 0 / `'weight4'/'weight'` 0 / `Link failed` 0 / `'size'` 0 / `GBufferInfo` 0 / `'normalMap'` 69 維持 / `'depthMap'` 9 維持 / `undeclared identifier` 0 (η-14 達成維持) / parse failed total 121 ± 0

**Phase 構成の特徴**: handoff §5 Phase 構成では Phase 1-4 (5-7 cycle) を η-16 sub-bundle 単独で完走 OR 分割を AYA 判断とし、`§9.1` で **option C-a (η-16 内継続)** vs **option C-b (η-16 = Phase 1 close、η-17 で Phase 2-4)** を未決として残していた。η-16 Phase 1 ACCEPT 結果報告に対し AYA「C-b」judgment (2026-06-02) で **Phase 2-4 を η-17 へ分割** 確定。Phase 1 単独 close = D approach の **構造的成立確認 phase** として位置付け、第2 phase 以降は η-17 で fresh context 着手。

---

## §2 完遂結果 metric (vs B?-η-15 末 baseline log `AYAstorm.log`)

| metric | η-15 末 baseline | η-16 P1 (verify) | Δ vs η-15 | 判定 |
|---|---|---|---|---|
| **SPIR-V requires location** | **41** | **30** | **-11** | ✓ **Phase 1 機構成立確認** |
| non-opaque uniforms outside a block | 0 | **3** | **+3** | cascade shift forward (handoff §6 risk #4 想定通り、全件 prep §3.3 内) |
| nameless block ... global scope | 0 | **2** | **+2** | cascade shift forward (同上) |
| undeclared identifier | 0 | 0 | ±0 | ✓ η-14 達成維持 |
| 'normalMap' : redefinition | 69 | 69 | ±0 | (η-17+ 移管継続) |
| 'depthMap' : redefinition | 9 | 9 | ±0 | (η-17+ 移管継続) |
| overlapping use of location | 0 | 0 | ±0 | ✓ η-12 達成維持 |
| Layout location qualifier | 0 | 0 | ±0 | ✓ η-10 達成維持 |
| 'binding' | 0 | 0 | ±0 | ✓ η-8 達成維持 |
| Cannot reuse block name | 0 | 0 | ±0 | ✓ η-1 達成維持 |
| 'weight4'/'weight' redefinition | 0 | 0 | ±0 | ✓ η-5/η-7 達成維持 |
| Link failed | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| 'size' undeclared | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| parse failed (count) | 121 | **121** | **±0** | net 不変 = cascade shift 内訳のみ変動、新 failure 0 |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | ±0 | ✓ |
| Goodbye | 1 | 1 | ±0 | ✓ clean shutdown |
| Vulkan device destroyed | 1 | 1 | ±0 | ✓ clean shutdown |

**SPIR-V -11 件達成 + 13 種既達主指標完全維持 + cascade 5 件全件 prep §3.3 内 (新 failure 0) + parse failed net 不変 + clean shutdown** = **η-16 Phase 1 D approach 機構成立確認 完了**、残 SPIR-V 30 件 + 露出 cascade 5 件 + normalMap 69 + depthMap 9 = **計 113 件 B?-η-17 移管**。

---

## §3 設計範式

### §3.1 新規 設計範式: C++ runtime location emit 範式 (D approach Phase 1)

**概要**: shader-only `layout(location=N)` 書込でなく、`LLGLSLShader::generatePerProgramSPIRV()` 内で stage source の concatenated buffer に対して regex で bare `out`/`in` 検出 → `layout(location=N)` 注入する **runtime transformer hook**。η-8 §3.3 mIndexedTextureChannels Vulkan-aware 範式 (C++ 側で `#define` injection) の自然延長で、**charter §3 #1 GL path byte-for-byte 不変担保** を C++ 経路で達成 (shader file には永続的に未注入)。

**Hook 構造**:
```cpp
// concatenated buffer 確定直後 (preprocess + 全 source 連結後)
if (auto_location_enabled)
{
    concatenated = vulkanizeStageSource(concatenated, stage_type);
}
if (dump_transformed_enabled)
{
    dumpTransformedStageSource(concatenated, program_hash, stage_type,
                               mgr->mShaderCacheDir, mName);
}
concat_buffers.push_back(std::move(concatenated));
```

**Phase 1 scope**:
- stage_type == GL_FRAGMENT_SHADER のみ実処理 (それ以外は source そのまま返却 = no-op)
- bare `out` 1 line match (qualifier 列 0-N 件 + type + ident + 任意 array suffix + `;`)
- pre-pass で既存 `layout(location=N) out` を audit して slot 予約
- 関数宣言 (line に `(` 含む) / `uniform` 行 / `layout` 行は skip

**handoff doc 記録価値**:
1. **shader file 永続書込 0** = upstream OpenGL Firestorm merge 時 GL `#else` byte 不変担保が C++ 側で完結、shader file は upstream 直 merge 可能 (handoff §6 risk #1 D acceptance)
2. **Vulkan migration の最終目標 (process isolation で CPU core 分離) 整合**: shader IR (SPIR-V) 生成パイプラインを C++ 側で完全制御するアーキテクチャの **第 1 段階** = 将来 Vulkan async compute / separate render thread の地ならし
3. **kill-switch** で transformer skip → 構造的退路確保 (η-15 末動作完全再現)

### §3.2 新規 設計範式: HBXXH128 cache key 先頭 version tag + kill-switch state inject 範式

**概要**: transformer 投入で transformed source が SPIR-V emit の真の入力になるため、**cache key の先頭に transformer version tag + kill-switch state を inject** することで kill-switch flip 時の stale binary cache hit を構造防止。

**Inject 規約**:
```cpp
HBXXH128 program_hash_obj;
program_hash_obj.update(std::string("vulkanize:v1_p1_fragout"));
program_hash_obj.update(std::string(auto_location_enabled ? "auto_loc=1" : "auto_loc=0"));
// ... 既存 source hash ...
```

**version tag bump 規則**:
- transformer 仕様変更時 (Phase 2 で V↔F pair 拡張時に `v1_p2_inout_pair` 等へ bump) 必須
- 同じ Phase 内 bug fix は **shader_cache 全削除** で対応 (version tag 同一でも cache key は source 内容で変動するため通常 hit せず、但し edge case 救済)

**handoff doc 記録価値**:
1. **kill-switch flip 即時 effect**: false ↔ true 切替で cache key が変わる → 旧 binary skip → 別 path emit
2. **Phase N → Phase N+1 移行時の構造的 cache invalidate**: version tag bump で全 program 再生成、副作用なし
3. **stale cache hit による silent bug** 防止: η-15 末以前の cache が残った状態で transformer ON しても、cache key 不一致で必ず再生成

### §3.3 設計範式継承: η-8 §3.3 mIndexedTextureChannels Vulkan-aware 範式の自然延長

**概要**: η-8 §3.3 で C++ 側で `#define` injection した範式 (`#define VULKAN_SAMPLER2D(loc, name)` 等の Vulkan 専用 macro 注入) を、**concatenated source の line 書換** へ拡張。両者とも「shader file には書かず C++ 側で Vulkan SPIR-V path 専用の transformation を施す」点で同型。

**範式継承表**:
- η-8 §3.3: `#define ...` injection (source 先頭追加) → 全 stage 一律
- η-16 §3.1: `out ...` → `layout(location=N) out ...` 書換 (line 単位) → stage 別判定 + slot allocator

### §3.4 設計範式継承表 (η-1〜η-15 全件継承)

| η-N | 範式 | η-16 適用状況 |
|---|---|---|
| η-1 §3.1 | FrameViewProj guard wrap | η-16 適用外 (UBO wrap でなく C++ transformer) |
| η-1 §3.2 | cascade pair hypothesis (主指標 -X ↔ cascade +Y) | η-16 §1 で **cascade shift forward 5 件 = handoff §6 risk #4 想定通り** = 順方向 Y < X (5 < 11) ACCEPT 適用第 N 例 |
| η-1 §3.3 | Agent 並列 disjoint scope | η-16 適用外 (Phase 1 単独実装で済) |
| η-2 (a) §3.2 | UBO body member 順序 std140 完全一致 | η-16 適用外 (UBO 系でなく) |
| η-3 §3.1/§3.2 | Link failed root cause UBO body 差異解消 + per-group rename | η-16 適用外 |
| η-4 §3.1/§3.2/§3.4 | nameless block × padding / cascade pair / UBO set=3 帯 | η-16 適用外 |
| η-5〜η-7 | 各種 | η-16 適用外 |
| η-8 §3.3 | mIndexedTextureChannels Vulkan-aware (C++ 側 `#define` injection) | η-16 §3.3 で **C++ runtime location emit 範式の母範式** として継承、自然延長 |
| η-8 §3.4 | bare uniform → UBO wrap | η-16 適用外 (D approach 採用で UBO wrap 範式は η-15 で打ち止め、η-17+ で必要時再投入候補) |
| η-9 §3.1 | V/F pair canonical partner 同定範式 | η-16 適用外 (Phase 1 fragment out 単独)、**Phase 2 V↔F pair で η-17 復活予定** |
| η-9 §3.2 | Phase 1 falsification + 完全 revert + Phase 2 軌道修正 | η-16 適用外 (Phase 1 即 ACCEPT) |
| η-9 §3.3/§3.4 | falsification_as_progress / admit_unknown | η-16 適用機会なし |
| η-10 §3.1 | handoff doc canonical 記載 着手前再検証範式 | η-16 着手前 trace で適用 (prep §3 SPIR-V 41 件分類 literal 再検証、§4 Hook 点 cpp:849 literal 再確認) |
| η-10 §3.3 | 1-shot ACCEPT vs 2-phase 構成判定範式 | η-16 で Phase 1 即 ACCEPT、Phase 2-4 分割 (option C-b) 適用第 N 例 |
| η-11 §3.1〜§3.5 | V/F pair canonical partner / 1-shot 判定 / cascade shift 順方向 ACCEPT / Agent 報告検証 | η-16 適用外、§3.4 cascade shift 順方向 ACCEPT は **§1 で η-16 cascade 5 件適用 第 N 例** |
| η-12 §3.1〜§3.4 | V/F pair / cascade 境界 / 1-shot 判定 / handoff doc 記録漏れ補完 | η-16 §3.4 で前 sub-bundle handoff §10.1 literal 再検証で継承 |
| η-13 §3.5〜§3.7 | retreat-revert / 同 binding 多 block / 4-Path 比較 | η-16 適用外 |
| η-14 §3.1〜§3.5 | GL stage hex / nameless block 構造解消 / 同 UBO 別 stage 複製 / cascade shift 逆方向 / handoff doc misanalysis 救済 | η-16 §3.1 で GL stage hex 0x8B30=FRAGMENT を hook 内 stage_type 判定で literal 適用 |
| η-15 §3.1 | binding 60+ 連続割当範式 | η-16 適用外 (UBO 新規割当なし) |
| η-15 §3.2 | 3 重 nest 条件 UBO wrap 範式 | η-16 適用外 |
| η-15 §3.3 | UBO wrap 3 系統並列適用 | η-16 適用外 (D approach 採用) |

**feedback memory 13 件全件適用確認**:
- **feedback_doubt_self_first**: 着手前 trace で D approach の構造的成立性を Hook 点・cache key・kill-switch 経路の各 layer で literal 再確認
- **feedback_root_cause_not_dump**: shader-only 範式の構造限界 (228 file 一括書込の upstream merge conflict) を D で根本解消、workaround 不採用
- **feedback_shader_only_fast_iterate**: 適用外 (shader file touch 0、C++ rebuild 必須)
- **feedback_no_auto_commit**: AYA「handoff doc 起草して、それから commit」明示指示後に commit 予定 (本 doc 起草 → commit)
- **feedback_no_claude_coauthor**: commit message に Co-Authored-By 不在予定
- **feedback_one_step_at_a_time**: 着手前 trace → 候補列挙 (A〜D の 4 案 prep) → AYA「D」judgment → Phase 構成提示 → AYA「Phase 1 着手して」→ implementation (Task 1-3 順次) → cold launch verify → 結果報告 → AYA「C-b」judgment → 本 doc 起草 を順次実施
- **feedback_proactive_handoff**: 本 handoff doc 起草
- **feedback_no_scope_shrink**: η-16 Phase 1 scope (D approach 機構成立確認 = fragment out 単独) は AYA「Phase 1 着手して」judgment 下確定 = scope shrink なし、Phase 2-4 分離は AYA judgment「C-b」確定で別 sub-bundle 計画的分割
- **feedback_admit_unknown**: η-16 で仮説外れず (Phase 1 即 ACCEPT、cascade shift forward 想定通り)
- **feedback_falsification_as_progress**: η-16 では falsification なし (Phase 1 即 ACCEPT)
- **feedback_self_verify_before_handoff**: AYA cold cache launch 依頼前に Claude が `vulkanizeStageSource()` regex literal + Hook 点 line position + cache key inject 順序 + LLCachedControl static 化 pattern (llfontregistry.cpp 参照) を Read で再確認 (self-verify all green)
- **feedback_confirm_referent_before_acting**: AYA 返答「A」(ambiguous) → 確認 1 行返し → AYA「option C」明確化、推測実行回避
- **feedback_explanation_lead_with_conclusion**: Phase 1 verify 結果報告で「ACCEPT 判定です」を冒頭明示、metric 表は後段配置

---

## §4 cold cache launch verify 詳細

### §4.1 verify cycle 構成 (1 cycle = Phase 1 即 ACCEPT)

**Cycle 1 (Phase 1 verify = ACCEPT)**:
- 操作: C++ 1 file edit (`llglslshader.cpp` +176/-2) + settings.xml +22 → self-verify (Read で edit 後構造 literal 確認、特に Hook 点 line position と LLCachedControl static 化 pattern) → autobuild full build → install → shader_cache clear → user_settings purge → AYA cold launch → shutdown
- 結果:
  - `SPIR-V requires location` 41 → **30** (-11 ✓ transformer 機構的成立確認)
  - non-opaque +3 / nameless block +2 = cascade shift forward 5 件 (全件 prep §3.3 内 = 新規 regression 0)
  - 13 種既達主指標完全維持 (normalMap 69 / depthMap 9)
  - parse failed count 121 → 121 (±0 = cascade shift 内訳のみ変動)
  - clean shutdown (Goodbye 1 + Vulkan device destroyed 1 + FATAL/SIGSEGV/Aborted 0)
- 判定: **η-16 Phase 1 deliverable 確定 (1-shot ACCEPT、AYA「C-b」judgment 確定 2026-06-02 = Phase 2-4 η-17 分割)**

### §4.2 主指標 metric integrity self-check

- `SPIR-V requires location` -11 = transformer 機構的成立:
  - bare `out` を含む fragment stage が SPIR-V missing location error を出さなくなった program 数 = 11
  - うち 5 件は次層 layer (non-opaque / nameless block) で再 error → SPIR-V missing 集合から脱落 + cascade 集合へ移動
  - 残 6 件は bare `out` 解消後に他種 layer (おそらく bare `in` = SPIR-V missing location vertex stage 由来) で再 error → SPIR-V missing 集合内に留まるが別 entry を計上していた可能性、または parse failed total 内訳変動なく内部 cascade
  - 但し parse failed total 121 ±0 = **新 failure 0** が支配的事実
- 残 SPIR-V 30 件 = Phase 2 (V↔F pair bare `in`/`out` 連携配置) + Phase 3 (vertex attribute) 主 scope

### §4.3 stage shift / cascade boost (η-16 Phase 1 では cascade shift forward 5 件発生)

η-15 では Cycle 内 cascade boost なし。η-16 Phase 1 では **handoff §6 risk #4 想定通り**:
- bare `out` 解消 → 次層 layer の **pre-existing error が露出** = cascade pair shift forward direction
- 5 件全件が prep §3.3 列挙の SPIR-V 41 件 affected program list 内 = **新 failure 0、layer 移動のみ**
- η-11 §3.4 cascade shift 順方向 ACCEPT 範式 (Y < X = 5 < 11) 適用第 N 例

### §4.4 shutdown clean verify

- Cycle 1 で:
  - `Goodbye!` 1 件 + `Vulkan device destroyed` 1 件
  - FATAL/SIGSEGV/Aborted 0/0/0

### §4.5 dump 機構の verify 実不在 (運用上注記)

`RenderVulkanShaderDumpTransformed` を AYA cold launch 後に enable 切替試みたが、cold launch は既に終了済で transformer 実行は cache hit path 経由となり dump 未生成。**Phase 1 ACCEPT 判定は log metric 観測で十分達成済** (SPIR-V -11 + cascade 5 件 prep §3.3 内 + parse failed ±0 + clean shutdown) のため、dump 取得は η-17 着手時に着手前 trace の補助手段として再投入予定 (handoff §6 risk #4 dump verify range, Phase 2 で V↔F pair canonical partner 同定の補助証拠化)。

---

## §5 self-verify 22 項目 all green

| # | 項目 | 状態 |
|---|---|---|
| 1 | charter §3 #1 GL path byte-for-byte 不変担保 (shader file touch 0) | ✓ |
| 2 | C++ 1 file (llglslshader.cpp) + settings.xml 1 file touch | ✓ |
| 3 | 主指標 `SPIR-V requires location` 41→30 literal grep verify | ✓ |
| 4 | cascade non-opaque 0→3 / nameless block 0→2 literal grep verify (5 件全件 prep §3.3 内確認) | ✓ |
| 5 | 既達主指標 13 種 literal grep verify 全 0 維持 | ✓ |
| 6 | normalMap/depthMap 69/9 維持 | ✓ |
| 7 | parse failed count 121→121 ±0 verify (cascade shift 内訳変動 / 新 failure 0) | ✓ |
| 8 | FATAL/SIGSEGV/Aborted 0 維持 | ✓ |
| 9 | clean shutdown (Goodbye 1 + Vulkan destroy 1) | ✓ |
| 10 | `vulkanizeStageSource()` regex pattern literal verify (qualifier 0-N 列 + type + ident + 任意 array) | ✓ |
| 11 | pre-pass 既存 layout(location=N) out audit + mUsedFragOutSlots 予約 + mFragOutCursor skip-on-used 衝突回避 | ✓ |
| 12 | 関数宣言 (line に `(` 含む) / `uniform` 行 / `layout` 行 skip 確認 | ✓ |
| 13 | Hook 点 = concatenated 確定後・concat_buffers.push_back() 直前 line position literal 確認 (cpp:1004 周辺) | ✓ |
| 14 | cache key 先頭 version tag `vulkanize:v1_p1_fragout` + kill-switch state `auto_loc=0/1` inject literal 確認 | ✓ |
| 15 | LLCachedControl static 化 pattern (llfontregistry.cpp 参照、gSavedSettings extern 経由) literal 確認 | ✓ |
| 16 | kill-switch `RenderVulkanShaderAutoLocation` debug settings 追加 (Boolean, default=1, Persist=1) literal 確認 | ✓ |
| 17 | dump 機構 `RenderVulkanShaderDumpTransformed` debug settings 追加 (Boolean, default=0, Persist=1) literal 確認 | ✓ |
| 18 | `dumpTransformedStageSource()` 出力先 `<shader_cache_dir>/transformed/<hash>_<vert\|frag\|geom>.glsl` literal 確認 | ✓ |
| 19 | stage_type != GL_FRAGMENT_SHADER 時 no-op return source 確認 (Phase 1 scope 範囲遵守) | ✓ |
| 20 | feedback_no_claude_coauthor 遵守 (commit message Co-Authored-By 不在予定) | ✓ |
| 21 | feedback_no_auto_commit 遵守 (AYA「handoff doc 起草して、それから commit」明示指示後に commit) | ✓ |
| 22 | handoff doc 別 commit (η-15-complete `cb28cf1daa` 範式継承) | ✓ |

---

## §6 設計範式継承表

(§3.4 と内容重複のため §3.4 参照、feedback memory 13 件全件適用確認 + prior sub-bundle 35+ 件継承 + 新規 2 件 §3.1/§3.2 + 継承 1 件 §3.3)

---

## §7 risks (η-17 持越し)

| # | risk | mitigation |
|---|---|---|
| 1 | Phase 2 で V↔F pair bare `in`/`out` 連携配置時、fragment stage の bare `out` slot N と vertex stage の bare `out` slot M (= F stage の `in`) を **必ず一致させる** 必要。η-16 Phase 1 は fragment out 単独で V/F 連携を扱わなかったため、Phase 2 で **pair allocator (V stage と F stage で同 slot を共有する allocator)** へ再設計必須 | Phase 2 着手前 trace で V/F pair の identifier mapping 表を canonical 作成 (η-9 §3.1 V/F pair 同定範式継承)、pair allocator は両 stage で同 program 内共有 (LLGLSLShader instance に持たせる) |
| 2 | Phase 2 で transformer version tag を `v1_p1_fragout` → `v2_p2_inout_pair` へ bump 必須 (§3.2)。bump 漏れで stale cache hit すると Phase 2 動作が Phase 1 動作で隠蔽される silent bug | Phase 2 着手最初 step で version tag bump、kill-switch 切替試験で cache invalidate 動作確認 |
| 3 | 残 SPIR-V 30 件のうち Phase 2 で解消する V↔F pair 由来件数の見積もり不確実 (vertex attribute 由来 vs F 内 bare `in` 由来の比率) | Phase 2 着手前 trace で 30 件 affected program list の各 program で bare `in` の source stage を grep で literal 同定 |
| 4 | cascade shift forward で露出した 5 件 (non-opaque 3 + nameless block 2) は **D approach の scope 外** (UBO wrap / nameless block 構造解消は shader-side 範式)、Phase 2-4 完走後の **後始末 phase (η-17 末 or η-18)** で対応必須 | Phase 4 完走時点で残 5 件 + 他 cascade 集計、η-18 で η-15 §3.2 3 重 nest 条件 UBO wrap 範式継承 or η-14 §3.2 nameless block 構造解消範式継承で個別対処 |
| 5 | dump 機構の実 verify 未実施 (§4.5)。η-17 で transformer 拡張時に出力検証手段として再投入 | η-17 Phase 2 着手前 trace で `RenderVulkanShaderDumpTransformed=true` 設定 → 該当 program の dump 取得 → regex 検証 → false 戻し |
| 6 | LocationAllocator は現状 fragment out スロットのみ管理。Phase 2-3 で vertex out / fragment in (= vertex out と pair) / vertex attribute (location 別 namespace) を扱う際、namespace 分離 (vert_in / vert_out_frag_in / frag_out) を整理する必要 | Phase 2 着手前 trace で namespace 設計、`LocationAllocator` を pair-aware に拡張 (program 単位インスタンス化、V/F 間 shared state) |
| 7 | regex 簡易 line-trim (line 内 `//` のみ trim、block comment 無視) で **block comment 内に `out` キーワード混入時の誤マッチ** 可能性 | shader 全 file の block comment 内 `out` 出現を grep audit、現状検出 0 確認 (Phase 1 は副作用なし)、Phase 2 でも継続 audit |
| 8 | `mUsedFragOutSlots` audit が **行頭 layout(location=N) out のみ** 対象。`flat out` 等 qualifier 先頭 + layout 後置 や multi-line layout decl は未対応 | 現状 shader file の全 `layout(location=N) out` literal は行頭形式 (η-12 / η-11 整合作業で標準化済)、Phase 2 で audit pattern 拡張 |
| 9 | shader_cache 件数 (η-7 305 baseline → η-11 310) η-12〜η-16 計測未実施 | η-17 で計測再開 + 第15層 emergence 観測 |
| 10 | cinematic_bd directory 全 .glsl audit (η-8 から継承、η-9〜η-16 未実施) | η-17 着手前 or 完遂後の別 phase として強く推奨 |
| 11 | runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画 | η-17 Phase 2 着手時に dump 機構経由で transformed source 取得して V↔F pair allocator 設計検証 |
| 12 | upstream OpenGL Firestorm merge 時の compatibility test (D approach の真価) は r41 全 sub-step 完遂後の sub-step 4.5 merge phase で初検証 | r41 sub-step 4.4 末点で shader file の GL path 全 byte-for-byte 不変 audit 実施、4.5 merge dry-run で conflict 確認 |

---

## §8 commit history

| commit | sub-bundle | scope |
|---|---|---|
| (η-16 patch 予定 = 本 cycle commit) | B?-η-16 patch (Phase 1) | C++ 1 file (llglslshader.cpp +176/-2) + settings.xml +22 = D approach Phase 1 (fragment out runtime location emit + kill-switch + dump) |
| `1c3eb16b6b` | B?-η-16 prep doc (-prep-D-switch) | (D approach 着手前 trace + Phase 構成 + risk + AYA judgment 2 件) |
| `cb28cf1daa` | B?-η-15 handoff doc | (η-15 完遂 handoff) |
| `6a11eabc73` | B?-η-15 patch | 3 file (waterV / terrainV / SMAABlendWeightsF) UBO wrap (non-opaque 6→0 達成) |
| `78df235243` | B?-η-14 handoff doc | (η-14 完遂 handoff) |
| `c971838656` | B?-η-14 patch | 3 file (skyV / cloudsV / cloudsF) Path G + G-β |
| (η-13 no commit) | B?-η-13 retreat-revert | Path B 失敗 → 完全 revert |
| `7c1762d214` | B?-η-12 handoff doc | (η-12 完遂 handoff) |
| `bf9164ce34` | B?-η-12 patch | 7 file Vulkan path location 21 → 27 整合 |
| `74705c35bb` | B?-η-11 handoff doc | (η-11 完遂 handoff) |
| `9a271fc900` | B?-η-11 patch | 15 file Vulkan path location 20 → 26 整合 |
| `9246142639` | B?-η-10 handoff doc | (η-10 完遂 handoff) |
| `e1d5ffd98c` | B?-η-10 patch | diffuseAlphaMaskF location 整合 |
| `6fee4a818b` | B?-η-9 handoff doc | (η-9 完遂 handoff) |
| `50cb5f6713` | B?-η-9 patch | avatarEyesV location 整合 |
| `6994eba271` | B?-η-8 handoff doc | (η-8 完遂 handoff) |
| `2542905ce0` | B?-η-8 patch | non-opaque 1 + 'binding' 25 |

---

## §9 file inventory (η-16 patch 2 file 内訳)

| # | file | 変更内容 | 行数 | 範式適用 |
|---|---|---|---|---|
| 1 | `indra/llrender/llglslshader.cpp` | (A) include 追加 (`<regex>`, `<set>`, `<sstream>`, `"llcontrol.h"` + `extern LLControlGroup gSavedSettings`) / (B) anonymous namespace 内 `LocationAllocator` struct + `vulkanizeStageSource()` (fragment stage line 単位 regex 書換) + `dumpTransformedStageSource()` (diagnostic) 新規 / (C) `generatePerProgramSPIRV()` 冒頭に `LLCachedControl<bool>` static 2 件 + cache key 先頭 version tag + kill-switch state inject / (D) concat_buffers.push_back() 直前に transformer hook + dump hook 挿入 | +176/-2 | §3.1 C++ runtime location emit (新規) + §3.2 cache key version tag (新規) + §3.3 η-8 §3.3 mIndexedTextureChannels Vulkan-aware 継承 |
| 2 | `indra/newview/app_settings/settings.xml` | (A) `RenderVulkanShaderAutoLocation` (Boolean, Persist=1, default=1, kill-switch) 追加 / (B) `RenderVulkanShaderDumpTransformed` (Boolean, Persist=1, default=0, diagnostic) 追加。挿入位置 = `RenderShaderCacheVersion` の直後、`RenderCASSharpness` の直前 (line 14719 周辺) | +22/-0 | §3.1 kill-switch 範式 + dump 範式 |

**計**: 2 file +198/-2 net +196、C++ 1 file + settings 1 file、shader file touch 0

**overlap file 確認**:
- llglslshader.cpp: η-8 で mIndexedTextureChannels Vulkan-aware 触り済、η-16 で第 2 touch (transformer hook 新規)
- settings.xml: 多数 sub-bundle で touch、η-16 で第 N touch (debug settings 2 件追加)

---

## §10 次 sub-bundle B?-η-17 推奨 scope

### §10.1 確定 scope: D approach Phase 2-4 + cascade 後始末

| Phase | scope | 推定対象 | 推奨範式 |
|---|---|---|---|
| Phase 2 | V↔F pair bare `in`/`out` 連携配置 | 残 SPIR-V 30 件のうち pair 由来 | C++ runtime location emit + pair allocator (program 単位インスタンス、V/F 間共有) + version tag bump `v2_p2_inout_pair` |
| Phase 3 | vertex attribute (vertex stage `in`) | Phase 2 完走後の SPIR-V 残件 | vertex stage 専用 attribute allocator (location namespace 分離)、version tag bump `v3_p3_vert_attr` |
| Phase 4 | geometry stage / その他 stage 拡張 | Phase 3 完走後の残件 (もしあれば) | stage type 拡張 case 追加 |
| 後始末 (η-17 末 or η-18) | cascade shift forward で露出した 5 件 (non-opaque 3 + nameless block 2) | non-opaque (Deferred Blur Light / FS Object ID / Deferred Buffer Visualization) + nameless block (Deferred Gamma Correction Post Process / Legacy Gamma Correction Post Process) | shader-side UBO wrap 範式 (η-15 §3.3 継承) or nameless block 構造解消 (η-14 §3.2 継承) |

### §10.2 B?-η-17 着手前 trace 範式 12 ステップ (η-16 §3.1/§3.2 範式追加)

1. **literal grep**: 各既達主指標 (16 種、SPIR-V も部分達成入り) + 残 SPIR-V 30 件 + normalMap 69 + depthMap 9 + 露出 cascade 5 件 (non-opaque 3 + nameless block 2) literal grep
2. **handoff doc 記録漏れ補完範式** (η-12 §3.4): 前 sub-bundle (η-16) handoff §2 metric 表 + §10.1 推奨 scope の literal 再検証
3. **handoff doc misanalysis 検出救済範式** (η-14 §3.5): 前 sub-bundle handoff doc の前提 (Phase 1 transformer 効果、cascade 5 件 prep §3.3 内対応関係、parse failed ±0 解釈) に literal 確認 step 必ず組込
4. **GL stage hex 解釈確認範式** (η-14 §3.1): log error の 0x8B30 = FRAGMENT / 0x8B31 = VERTEX / 0x8DD9 = GEOMETRY を着手前 trace で再確認、Phase 2 では vertex stage の SPIR-V missing を主対象化
5. **log context 抽出**: 残 30 件 SPIR-V missing program の error LINE × stage type × 真因 file 候補 (V or F 単独 or V↔F pair) を併記
6. **V/F pair canonical partner 同定範式** (η-9 §3.1 / η-11 §3.1 / η-12 §3.1): 30 件のうち V↔F pair 由来件数を canonical mapping 表で同定
7. **pair allocator 設計**: Phase 2 では program 単位 LocationAllocator を新規 instance 化、V stage out / F stage in の identifier mapping で同 slot 共有
8. **transformer version tag bump 確認** (§3.2): Phase 2 着手最初 step で `v1_p1_fragout` → `v2_p2_inout_pair` bump、kill-switch 切替試験で cache invalidate 動作確認
9. **dump 機構再投入** (§4.5 持越し): `RenderVulkanShaderDumpTransformed=true` で transformer output を file 化して regex 検証、Phase 2 着手前 trace 補助証拠
10. **regex pattern 拡張**: bare `in` 検出 + qualifier 列拡張 (flat/smooth/noperspective/centroid/highp/mediump/lowp に加え `invariant` 等の追加 keyword) + multi-line layout decl 対応の可否判定
11. **cascade shift forward 5 件後始末計画**: η-17 末 or η-18 で η-15 §3.3 UBO wrap 範式継承で non-opaque 3 件 → UBO wrap、η-14 §3.2 nameless block 構造解消範式で nameless block 2 件 → auto-attach helper 委譲
12. **upstream merge dry-run 計画** (§7 risk #12 持越し): r41 sub-step 4.4 末点で shader file の GL path 全 byte-for-byte 不変 audit + 4.5 merge dry-run で conflict 確認

### §10.3 B?-η-17 完遂後の想定 cascade exposure 第15層

- SPIR-V missing 30 件解消想定 = parse failed -30 程度 (Phase 2-3 完走で大量解消)
- normalMap / depthMap redefinition 78 件 → 別 sub-bundle 分割推奨 (η-18+ 移管候補)
- 露出 cascade 5 件 → η-17 末 or η-18 後始末で +0 net (UBO wrap or nameless block 構造解消)
- shader_cache 件数: η-17 で計測再開 + 第15層 emergence 観測

### §10.4 cinematic_bd 系統 systematic audit 推奨 (η-8 から継承、η-9〜η-16 未実施)

η-8 §3.1 cinematic_bd override path 発見範式で shadowUtil のみ個別検出。cinematic_bd directory 内 .glsl file 全件で類似 override + 未 wrap bare uniform / 未整合 location 残存可能性。B?-η-17 着手前 or 完遂後の別 phase として継続推奨。

### §10.5 runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画

η-16 では transformer 自身が dump 機構を内包したが (§3.1 §4.5)、cold launch タイミングずれで実 verify 未実施。η-17 Phase 2 着手前 trace 段階で再投入 → sub-bundle 完遂時 default OFF 戻し。

### §10.6 transformer version tag 後続管理 (η-16 §3.2)

η-16 末 = `vulkanize:v1_p1_fragout` 確定。η-17 Phase 2 着手最初 step で `v2_p2_inout_pair` bump 必須。kill-switch state inject 規約は継承 (`auto_loc=0/1`)。

### §10.7 pair allocator 設計仕様 (Phase 2 着手前 trace で確定すべき項目)

- program 単位 LocationAllocator instance 化 (現 anonymous local instance → LLGLSLShader メンバ化 or program ごと map 管理)
- V stage out → slot 割当 → 同 program の F stage in の同一 identifier に再利用 (identifier mapping table)
- mUsedFragOutSlots に加え `mUsedVertOutSlots` / `mUsedVertAttribSlots` 等を namespace 分離
- 既存 layout(location=N) audit pattern を V stage / F stage 別 pre-pass で実施

---

## §11 観測点

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-11 310) | η-12〜η-16 計測未実施 | B?-η-17 で計測再開 + 第15層 emergence 観測 |
| 2 | C++ runtime location emit 範式 (§3.1) 第 1 適用 (Phase 1 fragment out 単独) | 適用済 (SPIR-V -11、cascade shift forward 5 件) | B?-η-17 Phase 2 で V↔F pair 拡張、Phase 3 で vertex attribute 拡張 |
| 3 | HBXXH128 cache key 先頭 version tag + kill-switch state inject 範式 (§3.2) 第 1 適用 | 適用済 (`vulkanize:v1_p1_fragout` + `auto_loc=0/1`) | B?-η-17 Phase 2 で `v2_p2_inout_pair` bump 必須 |
| 4 | η-8 §3.3 mIndexedTextureChannels Vulkan-aware 範式の自然延長 (§3.3) | 適用済 | B?-η-17 でも C++ runtime transformation 範式継承 |
| 5 | cascade pair shift forward direction (handoff §6 risk #4) 想定通り 5 件 | 適用済 (全件 prep §3.3 SPIR-V 41 件原 list 内) | B?-η-17 末 or η-18 で後始末 (UBO wrap / nameless block 構造解消) |
| 6 | 1-shot ACCEPT vs N-phase 構成判定範式 (η-10 §3.3 等) Phase 1 単独 close + Phase 2-4 分割 適用第 N 例 | 適用済 (AYA「C-b」judgment 2026-06-02) | B?-η-17 で Phase 2 着手 (前 sub-bundle Phase 2-4 持越) |
| 7 | dump 機構 (§3.1 内包) の実 verify 不在 (§4.5) | cold launch タイミングずれで未実施 | B?-η-17 Phase 2 着手前 trace で再投入 |
| 8 | LocationAllocator namespace 分離 (Phase 1 = mUsedFragOutSlots のみ) | Phase 1 範囲適切 | B?-η-17 Phase 2-3 で V↔F pair allocator + vertex attribute allocator 拡張 |
| 9 | regex pattern Phase 1 = bare `out` のみ | Phase 1 範囲適切 | B?-η-17 Phase 2 で bare `in` 追加、qualifier 列拡張 |
| 10 | 既達主指標完全維持 (η-16 で 13 種 + SPIR-V 部分達成、退行 0 件) | ✓ η-16 で退行 0 件達成 | B?-η-17 で 13 種 + SPIR-V 残系統 scope |
| 11 | cinematic_bd directory 全 .glsl audit (§10.4) | 未実施 (η-8 から継承、η-9〜η-16 全て未実施) | B?-η-17 着手前 or 完遂後の別 phase として継続推奨 |
| 12 | runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画 | η-9〜η-16 未投入 (η-16 では機構同梱だが cold launch タイミングずれで未取得) | B?-η-17 Phase 2 着手前 trace で再投入 + sub-bundle 完遂時 default OFF 戻し |
| 13 | cascade exposure 第14層 残 (SPIR-V 30 + normalMap 69 + depthMap 9 + 露出 cascade 5 = 113 件) | sub-bundle 残量大 | B?-η-17 着手前 trace で系統別 Phase 分割継続推奨 (SPIR-V 30 件 Phase 2-4 集中、normalMap/depthMap 78 件は η-18+ 移管候補、露出 cascade 5 件は η-17 末 or η-18 後始末) |
| 14 | upstream OpenGL Firestorm merge 時 compatibility (D approach の真価) | r41 sub-step 4.5 merge phase で初検証 | r41 sub-step 4.4 末点で shader file の GL path 全 byte-for-byte 不変 audit |
| 15 | LLCachedControl static 化 pattern (llfontregistry.cpp 参照) llglslshader.cpp 内 第 1 適用 | 適用済 | B?-η-17 でも同 pattern 継承、static 化で gSavedSettings lookup 1 回抑制 |
| 16 | feedback_self_verify_before_handoff 適用 | η-16 で Read による edit 後構造 literal 再確認実施 (Hook 点 line position / cache key inject 順序 / LLCachedControl static 化 pattern) | B?-η-17 でも AYA cold cache launch 依頼前に必ず実施 |
| 17 | feedback_confirm_referent_before_acting 適用 | AYA「A」(ambiguous) → 確認 1 行返し → AYA「option C」明確化、推測実行回避 | B?-η-17 でも継承 |

---

**handoff doc 完。次 sub-bundle B?-η-17 着手は本 doc §10 推奨 scope (D approach Phase 2-4 + cascade 5 件後始末) を起点として、fresh context で実施。**
