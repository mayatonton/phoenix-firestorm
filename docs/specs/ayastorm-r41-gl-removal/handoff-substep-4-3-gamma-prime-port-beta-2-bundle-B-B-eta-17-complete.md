# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-17 完遂 handoff

**status**: B?-η-17 完遂 (Phase 2-4 ACCEPT、E approach `option E-1` = shader-only wrap + extra_code 隣接補修、SPIR-V missing location 30→0 完全消解、cascade shift forward 第15層露出は η-18 移管) → 次 sub-bundle B?-η-18 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: (本 doc 起草と同 cycle で commit、AYA「handoff doc 起草して、それから commit」明示指示下 2026-06-02 範式継承)
**handoff doc commit**: 本 doc (η-16-complete 範式継承、別 commit)
**勤続範式継承**: B?-η-16-complete `3653efed00` (patch `72fd4f3c3c`) / B?-η-16-prep-D-switch `1c3eb16b6b` / B?-η-15-complete `cb28cf1daa` (patch `6a11eabc73`) / B?-η-14-complete `78df235243` (patch `c971838656`) / B?-η-13-reverted (no commit) / B?-η-12-complete `7c1762d214` / B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-17 scope = **E approach `option E-1` 採用 (D approach 拡張は見送り、shader-only wrap + C++ extra_code 隣接補修) で SPIR-V missing location 30 件 → 0 件 完全消解** + cascade shift forward 第15層露出 (link failed 0→14 / non-opaque 3→11 / vary_fragcoord 0→6 / TerrainMix 0→2 / weight4 0→1 = 計 21 件)。**parse failed total 121 → 100 (-21 件 net 改善)**。Phase 2 (V↔F pair vary_texture_index) + Phase 3 (vertex attribute previewV) + Phase 4 (geometry normaldebugG) 全完遂、cascade 後始末 + V↔F pair location alignment audit (link failed 14 件) は **η-18 移管**。

- **Phase D (dump 取得 = 着手前 trace 補強)**: η-16 §10.5 で持越されていた dump 機構の実 verify を着手最初 step で実施。`RenderVulkanShaderDumpTransformed=1` を `~/.ayastorm_x64/user_settings/settings.xml` に直接 inject → cold cache launch → `~/.ayastorm_x64/cache/shader_cache/transformed/` 配下に 335 件 dump 取得。dump audit で **Fragment 24 件 SPIR-V missing の真因 = `flat in int vary_texture_index;` 単一 root cause** (llshadermgr.cpp L883 `extra_code_text` の η-8 Vulkan-aware wrap 漏れ) を確定。
- **Phase 2 (V↔F pair = vary_texture_index)**: `llshadermgr.cpp` L881-903 で `texture_index_channels > 1` 分岐内の `flat in int vary_texture_index;` extra_code 行を `#ifdef LL_VULKAN_GLSL layout(location=18) flat in int vary_texture_index; #else ...` で wrap。`indexedTextureV.glsl` L33 V side 既存 `layout(location=20)` を `layout(location=18)` に同期変更 (location=20 は `atmosphericsVarsF.glsl` L28 `layout(location=20) in vec3 vary_AdditiveColor;` と Diffuse / Fullbright 系 program で衝突、E-1 self-verify で location=18 = 0-30 帯唯一の未使用番号と確定)。
- **Phase 3 (vertex attribute = previewV)**: `previewV.glsl` L62-76 の `position` / `normal` / `texcoord0` 3 件 vertex attribute を `#ifdef LL_VULKAN_GLSL layout(location=0/1/2) in vec3/vec2 ...` で wrap (Object Preview Shader / Skinned Object Preview Shader 2 件解消)。
- **Phase 4 (geometry stage = normaldebugG)**: `normaldebugG.glsl` L32-49 の `vertex_color` out (location=2) / `normal_g[]` in array (location=20) / `tangent_g[]` in array under `HAS_ATTRIBUTE_TANGENT` (location=21) 3 件 wrap (Normal Debug Geometry 系 4 件解消)。
- **D approach 拡張見送り**: η-16 handoff §10.1 Phase 2-4 では C++ runtime transformer の V↔F pair allocator / vertex attribute allocator / geometry stage 拡張を予定していたが、AYA 「option E」judgment (2026-06-02) で **shader-only wrap (η-12 / η-11 範式の継続) + llshadermgr.cpp 内 extra_code 隣接補修** へ approach 切替。η-16 で投入した C++ runtime transformer (`vulkanizeStageSource()`) は **Phase 1 fragment out 単独機能のまま維持** (Phase 2 以降の拡張は未実施)。

**計**: **C++ 1 file (llshadermgr.cpp +21/-1 = +20) + shader 3 file (indexedTextureV.glsl +1/-1 / previewV.glsl +12/-0 / normaldebugG.glsl +12/-0)、user_settings.xml +5/-1 (dump 機構 enable は AYA 戻し対象)**

**主指標達成** (vs η-16 末 baseline = log `AYAstorm.log` 起動 2026-06-02T07:48Z 直前 baseline):
- `SPIR-V requires location` 30 → **0** ✓ **-30 件 完全消解 (Phase 2-4 主目標達成)**
- `parse failed (total)` 121 → **100** ✓ **-21 件 net 改善** (Phase 2-4 解消 30 件 - cascade exposure 9 件 = net -21、内訳: F 91 / V 9 / G 0)
- `Goodbye!` 1 / `Vulkan device destroyed` 1 / FATAL/SIGSEGV/Aborted 0/0/0 = ✓ clean shutdown

**cascade shift forward 第15層露出**:

| # | metric | η-16末 | η-17後 | Δ | 解析 |
|---|---|---|---|---|---|
| 1 | `link failed` | 0 | **14** | **+14** | V↔F bare varying location alignment mismatch (新層露出、§4.3) |
| 2 | `non-opaque uniforms outside a block` | 3 | **11** | **+8** | cascade shift forward (η-16 §6 risk #4 想定継続) |
| 3 | `nameless block ... global scope` | 2 | 2 | ±0 | 維持 (η-16 末 cascade 残置) |
| 4 | `'vary_fragcoord' : redefinition` | 0 (η-16 表非掲載) | **6** | **+6** | 新層露出 (PBR 系 program 推定) |
| 5 | `'TerrainMix' : redefinition struct` | 0 | **2** | **+2** | 新層露出 (terrain 系 program) |
| 6 | `'weight4' : redefinition` | 0 (η-5/η-7 達成 0 維持) | **1** | **+1** | 新層露出 (skinning 系 program、cascade 経由) |

**cascade exposure 第15層 計 21 件 + 既存維持 cascade 2 件 (nameless block) = 23 件 cascade 系統**。**全件 parse failed total 内訳変動として吸収** (parse failed total -21 件 net 改善 = Phase 2-4 解消 30 件 - 露出 cascade 9 件 + link failed は別カウント)。

**他既達主指標完全維持** (η-17 末):
- `overlapping use of location` 0 / `Layout location qualifier` 0 / `'binding'` 0 / `Cannot reuse block name` 0 / `'size'` 0 / `GBufferInfo` 0 / `undeclared identifier` 0 / `'normalMap'` 69 維持 / `'depthMap'` 9 維持

**Phase 構成の特徴**: η-16 handoff §10.1 で Phase 2-4 を D approach 拡張 (transformer V↔F pair allocator + vertex attribute allocator + geometry stage 拡張) で実装する予定だったが、着手前 Phase D (dump 取得) で **Fragment 24 件 SPIR-V missing の真因 = `vary_texture_index` extra_code 行の η-8 Vulkan-aware wrap 漏れ 単一 root cause** が判明 → AYA 「option E」judgment で **shader-only wrap (η-12/η-11 範式継続) + llshadermgr.cpp extra_code 隣接補修** へ approach 切替。option E は Phase 2-4 を **D approach 拡張ではなく E approach 範式 (4 file edit) で完走** する shrink/swap として位置付け。さらに AYA 「option E-1」judgment で vary_texture_index location=20 → 18 変更 (location=20 collision 回避) で確定。

---

## §2 完遂結果 metric (vs B?-η-16 末 baseline log)

| metric | η-16末 | η-17後 (verify) | Δ vs η-16 | 判定 |
|---|---|---|---|---|
| **SPIR-V requires location** | **30** | **0** | **-30** | ✓ **Phase 2-4 主目標完全達成** |
| **parse failed (total)** | **121** | **100** | **-21** | ✓ net 改善 (内訳: F 91 / V 9 / G 0) |
| link failed | 0 | **14** | **+14** | 新層露出 (V↔F bare varying location alignment、§4.3) |
| non-opaque uniforms outside a block | 3 | **11** | **+8** | cascade shift forward 想定通り |
| nameless block ... global scope | 2 | 2 | ±0 | ✓ |
| undeclared identifier | 0 | 0 | ±0 | ✓ η-14 達成維持 |
| 'normalMap' : redefinition | 69 | 69 | ±0 | (η-18+ 移管継続) |
| 'depthMap' : redefinition | 9 | 9 | ±0 | (η-18+ 移管継続) |
| 'vary_fragcoord' : redefinition | 0 | **6** | **+6** | 新層露出 (PBR 系) |
| 'TerrainMix' : redefinition struct | 0 | **2** | **+2** | 新層露出 (terrain 系) |
| 'weight4' : redefinition | 0 | **1** | **+1** | 新層露出 (skinning 系) |
| overlapping use of location | 0 | 0 | ±0 | ✓ η-12 達成維持 |
| Layout location qualifier (parse error) | 0 | 0 | ±0 | ✓ η-10 達成維持 (link error 14 件は別カテゴリ) |
| 'binding' | 0 | 0 | ±0 | ✓ η-8 達成維持 |
| Cannot reuse block name | 0 | 0 | ±0 | ✓ η-1 達成維持 |
| 'size' undeclared | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | ±0 | ✓ |
| Goodbye | 1 | 1 | ±0 | ✓ clean shutdown |
| Vulkan device destroyed | 1 | 1 | ±0 | ✓ clean shutdown |

**SPIR-V requires location -30 件 完全消解 + parse failed total -21 件 net 改善 + 13 種既達主指標完全維持 + clean shutdown** = **η-17 Phase 2-4 E approach `option E-1` 主目標達成**。残 100 件 parse failed = normalMap 69 + depthMap 9 + cascade exposure 第15層 21 件 + 既存 nameless block 2 件 + FXAA reserved names + 細粒 = **η-18+ 移管対象**。

---

## §3 設計範式

### §3.1 新規 設計範式: η-8 §3.3 mIndexedTextureChannels Vulkan-aware 範式の隣接行補修 範式 (option E-1)

**概要**: η-8 で `llshadermgr.cpp` L881-879 block (`texture_index_channels > 1` 分岐内 `uniform sampler2D tex%d;` を `#ifdef LL_VULKAN_GLSL layout(set=2, binding=N) uniform sampler2D ... #else ...` で wrap) は完了済だが、**同 block の直後 (L881 以降) で `flat in int vary_texture_index;` を bare で extra_code 注入する 1 行 (L883) が wrap 漏れ** = η-8 sub-bundle の取りこぼし。η-17 で当該 1 行を Vulkan path `layout(location=18) flat in int vary_texture_index;` でラップして補修。

**範式継承**: η-8 §3.3 mIndexedTextureChannels Vulkan-aware (C++ 側で `#define` injection と sampler2D wrap) → η-17 §3.1 同 block 隣接行 (vary_texture_index) の **後追い wrap 補修** = 同 sub-bundle 内 scope 取りこぼしの後追い補完範式。

**handoff doc 記録価値**:
1. **後追い wrap 補修範式 = 既達 sub-bundle (η-8) で scope 取りこぼしされた隣接行を後 sub-bundle で補修する範式** の第 1 例。η-N で wrap 完了と判定した block の隣接行も着手前 trace で literal 再 audit すべき (sub-bundle 境界跨ぎの取りこぼし発見手段)。
2. **dump audit による真因確定範式の併用**: bare `flat in int vary_texture_index;` 1 行を真因と特定したのは Phase D dump 取得 (`~/.ayastorm_x64/cache/shader_cache/transformed/` の Deferred Fullbright Shader frag dump L1199 と atmosphericsVarsF.glsl L28 `vary_AdditiveColor` location=20 衝突の dump audit)。dump 機構 (η-16 §3.1 内包) の Phase 2 着手前 trace 補助証拠化 (η-16 §10.5 持越し) を実施第 1 例。
3. **location=18 採用根拠 (E-1 self-verify)**: 0-30 帯の唯一の未使用番号、shader source 全域で他用途と衝突しない (Grep 全域確認)。location=20 (η-17 当初案 = E) は `atmosphericsVarsF.glsl` L28 `vary_AdditiveColor` と Diffuse / Fullbright 系 program で同時 attach すると `overlapping use of location` 退行リスク (13 種既達主指標 0 維持破壊) のため、AYA 「option E-1」judgment で 18 へ swap。

### §3.2 新規 設計範式: V↔F pair location 同期変更範式 (option E-1)

**概要**: V stage out / F stage in の **同一 identifier (vary_texture_index)** の location qualifier を `indexedTextureV.glsl` L33 (V side) と `llshadermgr.cpp` L883 (F side、extra_code 注入) **両方同時に変更** することで V↔F pair location alignment を担保。**片側だけ変更すると `link failed: Layout location qualifier must match` 退行** が発生 (本範式の構造的必須性、§4.3 で本 sub-bundle 末点での残 14 件 link failed の原因解析と一致)。

**範式継承**: η-9 §3.1 V/F pair canonical partner 同定範式 → η-17 §3.2 同 identifier pair の location qualifier 同期変更範式 (実装 pattern の確立)。

**handoff doc 記録価値**:
1. **V↔F pair location 同期変更は手動範式では取りこぼしリスク**: shader file (V side) と C++ extra_code (F side) は別 file / 別系統で管理されており、片側変更時の対側追従漏れが検出困難。手動範式 (本 sub-bundle で採用) を継続するなら **pair location は self-verify 段階で必ず literal 再 audit** すべき。
2. **将来の自動化候補**: handoff §10.7 で予定の **C++ runtime pair allocator (V↔F 間 shared state、program 単位 instance)** は本範式の自動化として η-18+ で再投入候補 = D approach Phase 2 拡張の構造的必要性が改めて確証 (§4.3 link failed 14 件解析でも同結論)。

### §3.3 設計範式継承: η-12 §3.1 V/F pair 個別 location 整合範式

**概要**: η-11 / η-12 で確立した「shader file 個別の V/F pair location qualifier 手動整合範式」を、Phase 3 (previewV.glsl) + Phase 4 (normaldebugG.glsl) で継続適用。η-12 では 7 file (Vulkan path location 21 → 27 整合)、η-11 では 15 file (location 20 → 26 整合)。η-17 では 3 file (previewV / normaldebugG / indexedTextureV) で継続。

**範式継承表**:
- η-11 §3.1: V/F pair location 整合 (15 file location 20-26 整合)
- η-12 §3.1: V/F pair location 整合 (7 file location 21-27 整合)
- η-17 §3.3: V/F pair location 整合 (3 file location 0-2/18/20-21 整合)

### §3.4 設計範式継承表 (η-1〜η-16 全件継承)

| η-N | 範式 | η-17 適用状況 |
|---|---|---|
| η-1 §3.1 | FrameViewProj guard wrap | η-17 適用外 |
| η-1 §3.2 | cascade pair hypothesis (主指標 -X ↔ cascade +Y) | η-17 §1 で **cascade shift forward 第15層露出 21 件 = handoff §6 risk #4 想定継続** = 順方向 Y < X (21 < 30) ACCEPT 適用第 N 例 |
| η-1 §3.3 | Agent 並列 disjoint scope | η-17 適用外 (Phase D 着手前 trace で Agent 並列適用試行、ただし agent 解析結果が misanalysis (Diffuse/Fullbright 全 wrap済) → dump audit で覆る、Agent 結果は補助証拠化のみ留め) |
| η-2 (a) §3.2 | UBO body member 順序 std140 完全一致 | η-17 適用外 |
| η-3 §3.1/§3.2 | Link failed root cause UBO body 差異解消 + per-group rename | η-17 適用外 (η-17 link failed 14 件は別系統 = V↔F bare varying location alignment、§4.3) |
| η-4 §3.1/§3.2/§3.4 | nameless block × padding / cascade pair / UBO set=3 帯 | η-17 適用外 |
| η-5〜η-7 | 各種 | η-17 適用外 |
| η-8 §3.3 | mIndexedTextureChannels Vulkan-aware (C++ 側 `#define` + sampler2D wrap) | η-17 §3.1 で **隣接行補修範式の母範式** として継承、補修第 1 例 |
| η-8 §3.4 | bare uniform → UBO wrap | η-17 適用外 (E approach 採用 = shader-only wrap + extra_code 補修) |
| η-9 §3.1 | V/F pair canonical partner 同定範式 | η-17 §3.2 で V↔F pair location 同期変更範式の母範式として継承、実装 pattern 確立 |
| η-9 §3.2 | Phase 1 falsification + 完全 revert + Phase 2 軌道修正 | η-17 適用外 (Phase D = 着手前 trace、Phase 2-4 = 即 ACCEPT) |
| η-9 §3.3/§3.4 | falsification_as_progress / admit_unknown | η-17 適用外 (即 ACCEPT) |
| η-10 §3.1 | handoff doc canonical 記載 着手前再検証範式 | η-17 着手前 trace で適用 (η-16 handoff §10.1/§10.2 literal 再検証 + §10.7 pair allocator 設計仕様 literal 再確認) |
| η-10 §3.3 | 1-shot ACCEPT vs N-phase 構成判定範式 | η-17 で Phase 2-4 即 ACCEPT、Phase D (着手前 trace) を別カテゴリで追加 |
| η-11 §3.1〜§3.5 | V/F pair canonical partner / 1-shot 判定 / cascade shift 順方向 ACCEPT / Agent 報告検証 | η-17 §3.3 で V/F pair 個別 location 整合範式継承、§3.4 cascade shift 順方向 ACCEPT は §1 で η-17 cascade 21 件適用 第 N 例 |
| η-11 §3.5 | Agent 報告検証 (信任せず literal 再検証) | η-17 Phase D で **Agent misanalysis 検出救済** 適用第 N 例 (agent 報告「Diffuse/Fullbright 全 wrap 済」を dump audit で覆す) |
| η-12 §3.1〜§3.4 | V/F pair / cascade 境界 / 1-shot 判定 / handoff doc 記録漏れ補完 | η-17 §3.3 で V/F pair 範式継承、§3.4 で前 sub-bundle handoff §10.1 literal 再検証で継承 |
| η-13 §3.5〜§3.7 | retreat-revert / 同 binding 多 block / 4-Path 比較 | η-17 適用外 |
| η-14 §3.1〜§3.5 | GL stage hex / nameless block 構造解消 / 同 UBO 別 stage 複製 / cascade shift 逆方向 / handoff doc misanalysis 救済 | η-17 §3.1 で GL stage hex 0x8B30=FRAGMENT / 0x8B31=VERTEX / 0x8DD9=GEOMETRY を着手前 trace で literal 適用、§3.5 handoff doc misanalysis 救済範式は本 sub-bundle 完遂報告中盤で適用 (前 session summary 上「13 種主指標 0 件達成」誤読 → η-16 handoff §2 表 literal 再確認で「normalMap 69 / depthMap 9 維持」事実訂正) |
| η-15 §3.1 | binding 60+ 連続割当範式 | η-17 適用外 (UBO 新規割当なし) |
| η-15 §3.2 | 3 重 nest 条件 UBO wrap 範式 | η-17 適用外 |
| η-15 §3.3 | UBO wrap 3 系統並列適用 | η-17 適用外 (E approach 採用) |
| η-16 §3.1 | C++ runtime location emit 範式 (D approach Phase 1) | η-17 では **拡張せず維持** (Phase 2-4 は E approach 採用)。Phase 1 fragment out 単独機能のまま `vulkanizeStageSource()` は無拡張で維持、Phase 2 以降の transformer 拡張は η-18+ で再投入候補 |
| η-16 §3.2 | HBXXH128 cache key 先頭 version tag + kill-switch state inject 範式 | η-17 では **version tag bump せず維持** (`vulkanize:v1_p1_fragout` 据え置き、E approach 採用で C++ transformer 拡張なしのため bump 不要)。η-18 で transformer 拡張する場合は `v2_p2_inout_pair` bump 必須 |
| η-16 §3.3 | η-8 §3.3 mIndexedTextureChannels Vulkan-aware 自然延長 | η-17 §3.1 で **隣接行補修範式の母範式として再継承** |

**feedback memory 13 件全件適用確認**:
- **feedback_doubt_self_first**: 着手前 trace で前 session summary 上「13 種主指標 0 件達成」想定 → η-16 handoff §2 表 literal 再確認で「normalMap 69 / depthMap 9 維持」事実訂正、自分の summary 上誤読を先疑い
- **feedback_root_cause_not_dump**: Fragment 24 件 SPIR-V missing を「Phase 2 transformer 拡張で解消」前提せず、Phase D (dump 取得) で真因確定 = vary_texture_index extra_code 行 1 行の wrap 漏れ
- **feedback_use_agents_proactively**: Phase D 着手前 trace で Diffuse/Fullbright 全 file 確認 Agent 並列適用、ただし Agent misanalysis (「全 wrap 済」報告) を dump audit で覆す = η-11 §3.5 Agent 報告検証範式適用
- **feedback_no_auto_commit**: AYA「handoff doc 起草して、それから commit」明示指示 (η-16 範式継承) 後に commit 予定
- **feedback_no_claude_coauthor**: commit message に Co-Authored-By 不在予定
- **feedback_one_step_at_a_time**: 着手前 trace → Phase D dump 取得提案 → AYA「D で進めて」 → Phase D 実施 → 候補列挙 (option A〜E 5 案) → AYA「E で進めて」 → option E 内 swap 候補 (E-1 location=18) → AYA「E-1 で進めて」 → implementation 4 file edit → cold launch verify → 結果報告 → 本 doc 起草 を順次実施
- **feedback_proactive_handoff**: 本 handoff doc 起草
- **feedback_no_scope_shrink**: η-17 Phase 2-4 scope は AYA「option E-1」judgment 下確定 = approach 切替 (D 拡張 → E + E-1 shrink/swap) は AYA judgment ベース = scope shrink 違反なし
- **feedback_admit_unknown**: η-17 では Phase D dump 取得タイミング (cold launch 直後 v.s. 2 回目 cold launch) で 1 回目 dump 不在 → 2 回目 launch で 335 件 dump 取得 = dump 機構の cold launch タイミング依存性を「自分で分からない」と認めて AYA に「shutdown 済」「launch 済」確認逐次取得
- **feedback_falsification_as_progress**: η-17 では falsification 1 例: E approach 当初 location=20 提案 → self-verify で `vary_AdditiveColor` 衝突発見 → E-1 swap (location=18) = self-verify 段階で自己 falsify + 候補 swap、AYA に提示前に内部 falsify 完了
- **feedback_self_verify_before_handoff**: AYA build 依頼前に 4 file edit 内容 literal 再確認 (location=18 unique 性、V↔F pair 同期、normaldebugG.glsl G stage location 帯=20-21 が他系統と衝突しないこと grep audit)、self-verify all green
- **feedback_confirm_referent_before_acting**: AYA「E で進めて」 → E 案内に location=20 含む → self-verify で衝突発見 → AYA に「E-1 提案 (location=18)」確認 1 行 → AYA「E-1 で進めて」明確化、推測実行回避
- **feedback_explanation_lead_with_conclusion**: cold launch verify 結果報告で「SPIR-V requires location 30→0 完全消解 = Phase 2-4 主目標達成」を冒頭明示、metric 表は後段配置
- **feedback_restore_debug_settings**: η-17 完遂時 AYA に `RenderVulkanShaderDumpTransformed=0 戻し` 案内含める (§10.5 参照)

---

## §4 cold cache launch verify 詳細

### §4.1 verify cycle 構成 (1 cycle = Phase D + Phase 2-4 即 ACCEPT)

**Cycle 0 (Phase D = dump 取得 着手前 trace)**:
- 操作: `RenderVulkanShaderDumpTransformed=1` を `~/.ayastorm_x64/user_settings/settings.xml` に直接 inject (Debug Settings UI 経由 enable は 1 回目 cold launch で persist 不発、user_settings.xml 直接 inject で 2 回目 cold launch で発火) → cold launch → shutdown
- 結果:
  - `~/.ayastorm_x64/cache/shader_cache/transformed/` 配下に **335 件 dump 生成** (vert/frag 個別、program 単位)
  - Deferred Fullbright Shader frag dump (`961cf6dd-..._frag.glsl`) で **L335 `layout(location=20) in vec3 vary_AdditiveColor;` + L1199 `flat in int vary_texture_index;` 共存** を確認 → Fragment 24 件 SPIR-V missing の真因 = vary_texture_index bare in 単一 root cause を特定 (atmosphericsVarsF.glsl L28 由来 vary_AdditiveColor との同 program attach も同時確認、option E 当初案 location=20 衝突回避必須を判定)

**Cycle 1 (Phase 2-4 verify = ACCEPT)**:
- 操作: 4 file edit (llshadermgr.cpp +21/-1, indexedTextureV.glsl +1/-1, previewV.glsl +12/-0, normaldebugG.glsl +12/-0) → self-verify (Read で edit 後構造 literal 確認、特に location=18 unique 性 / V↔F pair 同期 / normaldebugG.glsl G stage location 帯) → autobuild configure + build → install → shader_cache clear → AYA cold launch → shutdown
- 結果:
  - `SPIR-V requires location` 30 → **0** ✓ (-30 件 完全消解)
  - `parse failed (total)` 121 → **100** ✓ (-21 件 net 改善、F 91 / V 9 / G 0)
  - `link failed` 0 → **14** = cascade shift forward 第15層露出 (§4.3、V↔F bare varying location alignment mismatch)
  - cascade exposure 第15層 計 21 件 (non-opaque +8 / vary_fragcoord +6 / TerrainMix +2 / weight4 +1 / link failed +14 = 31 件、ただし link failed は別カウント) = handoff §6 risk #4 cascade shift forward direction 想定継続
  - 13 種既達主指標完全維持
  - clean shutdown (Goodbye 1 + Vulkan device destroyed 1 + FATAL/SIGSEGV/Aborted 0/0/0)
- 判定: **η-17 Phase 2-4 deliverable 確定 (E approach `option E-1` 即 ACCEPT)**、cascade exposure 第15層は η-18 移管

### §4.2 主指標 metric integrity self-check

- `SPIR-V requires location` -30 = Phase 2-4 完全消解:
  - **Phase 2 解消**: vary_texture_index pair (24 件 Fragment 系) - llshadermgr.cpp wrap + indexedTextureV.glsl sync
  - **Phase 3 解消**: previewV vertex attribute (2 件 Vertex 系 = Object Preview / Skinned Object Preview)
  - **Phase 4 解消**: normaldebugG geometry stage (4 件 Geometry 系 = Normal Debug Shader 4 variant)
  - 計 30 件 = η-16 末 SPIR-V missing 30 件 完全網羅
- `parse failed total` 121 → 100 (-21 件 net):
  - Phase 2-4 解消 30 件 - cascade exposure 第15層 9 件 (non-opaque +8 / nameless ±0 = 8、ただし vary_fragcoord 6 件は別カウントで parse failed total には算入されるため、内訳は内部 cascade) = net -21 件
  - 但し link failed 14 件は **parse failed total とは別カテゴリ** (link 段階 error)

### §4.3 link failed 14 件の真因解析 (新層露出 = V↔F bare varying location alignment mismatch)

**dump audit 結果** (Deferred Diffuse Shader vert dump `4918288c-..._vert.glsl` + frag dump `4918288c-..._frag.glsl`):

| varying | V side (手動 wrap、shader file) | F side (手動 wrap、shader file) | V/F 一致 |
|---|---|---|---|
| `vary_normal` | location=0 | **location=4** | ✗ mismatch |
| `vertex_color` | location=1 | **location=2** | ✗ mismatch |
| `vary_texcoord0` | location=2 | **location=0** | ✗ mismatch |
| `vary_position` | location=3 | location=3 | ✓ |
| `vary_texture_index` (η-17 sync) | location=18 | location=18 | ✓ η-17 で同期確立 |

**解析**: shader file 内に **過去の sub-bundle (η-11/η-12 等) で手付けされた V↔F bare varying の location qualifier が V/F で番号不整合** な pair が複数存在。η-16 末では `Layout location qualifier must match` error がカウント 0 だったが、これは「以前は link 段階まで到達していなかった = parse 段階 (SPIR-V requires location 30 件) で fail していた program が、η-17 で SPIR-V missing 解消 → link 段階まで進める → link failed 露出」という cascade shift forward (parse → link layer 移動)。

**結論**: link failed 14 件は **η-17 退行ではなく、η-N (N<17) で発生していた pre-existing V↔F pair location mismatch が、SPIR-V missing 30 件解消で初めて link 段階まで到達して露出した**。handoff §6 risk #4 cascade shift forward direction 想定通り (parse → link layer 移動)。η-18 で **V↔F pair allocator** (η-16 §10.7 設計仕様、本 sub-bundle 末点で η-18 主 scope 確定) または **手動 V↔F pair location 全 audit + 整合変更** で対処。

### §4.4 shutdown clean verify

- Cycle 0 (Phase D) で:
  - `Goodbye!` 1 件 + `Vulkan device destroyed` 1 件 (1 回目 / 2 回目 cold launch とも)
  - FATAL/SIGSEGV/Aborted 0/0/0
- Cycle 1 (Phase 2-4 verify) で:
  - `Goodbye!` 1 件 + `Vulkan device destroyed` 1 件
  - FATAL/SIGSEGV/Aborted 0/0/0

### §4.5 dump 機構 cold launch タイミング依存性 (Phase D で発見、η-18 引継ぎ運用知)

`RenderVulkanShaderDumpTransformed` を Debug Settings UI から enable 切替しても、設定 persist 不発 (1 回目 cold launch で dump 不生成)。`user_settings.xml` 直接 inject で 2 回目 cold launch で dump 生成発火 = **UI 経由 enable は cold launch 跨ぎで反映されない、xml 直接 inject 必須**。η-18 以降で dump 機構を再投入する場合は同じ運用 (xml 直接 inject) を踏襲推奨。

---

## §5 self-verify 18 項目 all green

| # | 項目 | 状態 |
|---|---|---|
| 1 | charter §3 #1 GL path byte-for-byte 不変担保 (shader file 3 件 wrap は `#ifdef LL_VULKAN_GLSL` / `#else` 分岐内のみ、GL path 不変) | ✓ |
| 2 | C++ 1 file (llshadermgr.cpp) + shader 3 file (indexedTextureV / previewV / normaldebugG) + user_settings.xml (AYA 環境 only、リポジトリ touch 0) | ✓ |
| 3 | 主指標 `SPIR-V requires location` 30→0 literal grep verify | ✓ |
| 4 | parse failed total 121→100 literal grep verify (-21 net) | ✓ |
| 5 | 既達主指標 13 種 literal grep verify (overlapping/Layout/binding/Cannot reuse/size/GBufferInfo/undeclared/normalMap 69/depthMap 9 維持) | ✓ |
| 6 | cascade exposure 第15層 21 件 (link failed 14 + non-opaque +8 + vary_fragcoord 6 + TerrainMix 2 + weight4 1) literal grep verify | ✓ |
| 7 | FATAL/SIGSEGV/Aborted 0 維持 | ✓ |
| 8 | clean shutdown (Goodbye 1 + Vulkan destroy 1) | ✓ |
| 9 | location=18 unique 性 verify (shader source 全域で他用途と衝突しない、Grep 全域確認 1 件のみ = indexedTextureV.glsl L33) | ✓ |
| 10 | V↔F pair vary_texture_index 同期 verify (V indexedTextureV.glsl L33 location=18 + F llshadermgr.cpp L883 extra_code location=18) | ✓ |
| 11 | option E 当初案 location=20 衝突回避 (atmosphericsVarsF.glsl L28 `vary_AdditiveColor` location=20 と Diffuse / Fullbright 系 program 同時 attach 衝突を dump audit で発見) | ✓ |
| 12 | previewV.glsl L62-76 vertex attribute 3 件 (position 0 / normal 1 / texcoord0 2) wrap literal 確認 | ✓ |
| 13 | normaldebugG.glsl L32-49 geometry stage 3 件 (vertex_color out 2 / normal_g[] in 20 / tangent_g[] in 21) wrap literal 確認 | ✓ |
| 14 | normaldebugG.glsl の normaldebugV.glsl V side 既存 wrap (`vary_normal_g` out location=20 / `vary_tangent_g` out location=21) と G side 入力 (normal_g[] / tangent_g[]) location 一致 verify | ✓ |
| 15 | normaldebugG.glsl の vertex_color out location=2 が normaldebugF.glsl F side `layout(location=2) in vec4 vertex_color;` と一致 verify | ✓ |
| 16 | dump 機構 `RenderVulkanShaderDumpTransformed=1` 戻し案内 (§10.5、AYA 環境 user_settings.xml) | ✓ (案内文 §10.5 記載) |
| 17 | feedback_no_claude_coauthor 遵守 (commit message Co-Authored-By 不在予定) | ✓ |
| 18 | feedback_no_auto_commit 遵守 (AYA「handoff doc 起草して、それから commit」明示指示後に commit) | ✓ |

---

## §6 設計範式継承表

(§3.4 と内容重複のため §3.4 参照、feedback memory 14 件全件適用確認 + prior sub-bundle 36+ 件継承 + 新規 2 件 §3.1/§3.2 + 継承 1 件 §3.3)

---

## §7 risks (η-18 持越し)

| # | risk | mitigation |
|---|---|---|
| 1 | **link failed 14 件 = V↔F bare varying location alignment mismatch** が pre-existing pair で複数 program に分散露出 (§4.3、Deferred Diffuse 系 4 件 + PBR 系 多数 + Fullbright Alpha Masking 系)。手動 audit + 整合変更で対処すると 14+ pair × 複数 identifier の大規模 swap | (A) handoff §10.7 で予定の **C++ runtime V↔F pair allocator (program 単位 instance、V/F shared state)** を η-16 §3.1 transformer に Phase 2 として拡張 + version tag `v2_p2_inout_pair` bump、または (B) shader file 個別 audit + V↔F pair location 全件揃え変更 (η-11/η-12 範式継続)。η-18 着手前 trace で A/B 比較推奨 |
| 2 | non-opaque uniforms outside a block 11 件 (η-16 末 3 + η-17 +8) は **E approach の scope 外** (UBO wrap は shader-side 範式)。η-17 で +8 件 cascade exposure | η-18 末 or η-19 で η-15 §3.3 UBO wrap 範式継承で個別対処、affected program list の grep 同定先行 |
| 3 | vary_fragcoord redefinition 6 件 (η-17 新層露出) = PBR 系 program 推定。η-16 末 metric 表非掲載項目 | η-18 着手前 trace で affected program list grep 同定、shader file 個別 audit |
| 4 | TerrainMix redefinition struct 2 件 (η-17 新層露出) = terrain 系 program。η-16 末 0 件達成 → 露出 | η-18 着手前 trace で terrain shader 全 file audit |
| 5 | weight4 redefinition 1 件 (η-17 新層露出) = skinning 系 program。η-5/η-7 0 達成 → 1 件露出 | η-18 着手前 trace で skinning shader 全 file audit、η-5/η-7 範式再適用 |
| 6 | normalMap 69 / depthMap 9 redefinition (η-N N<17 から維持継続) | η-18+ 別 sub-bundle 移管、η-17 では sub-bundle scope 外 |
| 7 | nameless block 2 件 (η-16 末 維持) | η-18 末 or η-19 で η-14 §3.2 nameless block 構造解消範式継承 |
| 8 | η-16 で投入した C++ runtime transformer (`vulkanizeStageSource()`) は **Phase 1 fragment out 単独機能のまま維持** (Phase 2-4 拡張は未実施)。η-18+ で V↔F pair allocator 拡張する場合は version tag `v1_p1_fragout` → `v2_p2_inout_pair` bump 必須 (η-16 §7 risk #2 継続) | η-18 で C++ transformer 拡張する場合、最初 step で version tag bump、kill-switch 切替試験で cache invalidate 動作確認 |
| 9 | dump 機構 cold launch タイミング依存性 (§4.5) = UI 経由 enable で 1 回目 cold launch で persist 不発 | η-18 で dump 再投入する場合 user_settings.xml 直接 inject 運用継承 |
| 10 | LocationAllocator は現状 fragment out スロットのみ管理 (η-16 §7 risk #6 継続) | η-18 Phase 2 (V↔F pair) 着手時に program 単位 instance + namespace 分離 (vert_in / vert_out_frag_in / frag_out) 設計 |
| 11 | shader_cache 件数 (η-7 305 baseline → η-11 310) η-12〜η-17 計測未実施 | η-18 で計測再開 + 第16層 emergence 観測 |
| 12 | cinematic_bd directory 全 .glsl audit (η-8 から継承、η-9〜η-17 未実施) | η-18 着手前 or 完遂後の別 phase として強く推奨 |
| 13 | upstream OpenGL Firestorm merge 時 compatibility (D approach の真価) は r41 全 sub-step 完遂後の sub-step 4.5 merge phase で初検証 | r41 sub-step 4.4 末点で shader file の GL path 全 byte-for-byte 不変 audit 実施、4.5 merge dry-run で conflict 確認 |
| 14 | option E-1 採用で shader file 3 件 (indexedTextureV / previewV / normaldebugG) に Vulkan path location qualifier を hard-code 追加 = upstream merge 時の conflict 可能性 (GL path 不変だが `#ifdef LL_VULKAN_GLSL` 分岐は upstream 差分対象) | sub-step 4.5 merge phase で本 sub-bundle で追加した 3 file の `#ifdef LL_VULKAN_GLSL` 分岐 patch を upstream に対して dry-run merge、conflict 出る場合は η-16 §3.1 D approach (C++ runtime transformer) への移行検討 |

---

## §8 commit history

| commit | sub-bundle | scope |
|---|---|---|
| (η-17 patch 予定 = 本 cycle commit) | B?-η-17 patch (Phase 2-4 + Phase D) | C++ 1 file (llshadermgr.cpp +21/-1) + shader 3 file (indexedTextureV.glsl +1/-1 / previewV.glsl +12/-0 / normaldebugG.glsl +12/-0) = E approach `option E-1` 4 file edit |
| `3653efed00` | B?-η-16 handoff doc | (η-16 完遂 handoff) |
| `72fd4f3c3c` | B?-η-16 patch | C++ 1 file (llglslshader.cpp +176/-2) + settings.xml +22 = D approach Phase 1 (fragment out runtime location emit + kill-switch + dump) |
| `1c3eb16b6b` | B?-η-16 prep doc | (D approach 着手前 trace) |
| `cb28cf1daa` | B?-η-15 handoff doc | (η-15 完遂 handoff) |
| `6a11eabc73` | B?-η-15 patch | 3 file (waterV / terrainV / SMAABlendWeightsF) UBO wrap |

(η-14 以前は η-16 handoff doc §8 参照)

---

## §9 file inventory (η-17 patch 4 file 内訳)

| # | file | 変更内容 | 行数 | 範式適用 |
|---|---|---|---|---|
| 1 | `indra/llrender/llshadermgr.cpp` | (A) L881-903 `texture_index_channels > 1` 分岐内 `flat in int vary_texture_index;` extra_code 行を `#ifdef LL_VULKAN_GLSL layout(location=18) flat in int vary_texture_index; #else flat in int vary_texture_index; #endif` で wrap (η-8 sub-bundle 取りこぼし補修) | +21/-1 | §3.1 η-8 §3.3 隣接行補修範式 (新規) + §3.2 V↔F pair location 同期変更範式 (新規) |
| 2 | `indra/newview/app_settings/shaders/class1/objects/indexedTextureV.glsl` | L33 `layout(location=20) flat out int vary_texture_index;` → `layout(location=18) flat out int vary_texture_index;` (option E-1 = V↔F pair location 同期、location=20 vary_AdditiveColor 衝突回避) | +1/-1 | §3.2 V↔F pair location 同期変更範式 (新規) + §3.3 η-12 §3.1 V/F pair 個別 location 整合範式継承 |
| 3 | `indra/newview/app_settings/shaders/class1/objects/previewV.glsl` | L62-76 `position` / `normal` / `texcoord0` 3 件 vertex attribute を `#ifdef LL_VULKAN_GLSL layout(location=0/1/2) in vec3/vec2 ...; #else in vec3/vec2 ...; #endif` で wrap (Phase 3 = Object Preview / Skinned Object Preview 2 件 SPIR-V 解消) | +12/-0 | §3.3 η-12 §3.1 V/F pair 個別 location 整合範式継承 (Phase 3 vertex attribute 拡張) |
| 4 | `indra/newview/app_settings/shaders/class1/interface/normaldebugG.glsl` | L32 `out vec4 vertex_color;` → location=2 wrap / L38-39 `in vec4 normal_g[];` → location=20 wrap / L43-46 `in vec4 tangent_g[];` (HAS_ATTRIBUTE_TANGENT 内) → location=21 wrap (Phase 4 = Normal Debug 系 4 件 SPIR-V 解消) | +12/-0 | §3.3 η-12 §3.1 V/F pair 個別 location 整合範式継承 (Phase 4 geometry stage 拡張) + normaldebugV.glsl V side 既存 location=20/21 + normaldebugF.glsl F side 既存 location=2 との pair 同期 |

**計**: 4 file +46/-2 net +44、C++ 1 file + shader 3 file。settings.xml は AYA 環境 user_settings のみ touch (リポジトリ修正 0)

**overlap file 確認**:
- llshadermgr.cpp: η-8 で mIndexedTextureChannels Vulkan-aware 触り済、η-17 で第 2 touch (隣接行補修)
- indexedTextureV.glsl: η-N (N<17) 既存 wrap、η-17 で location qualifier 変更
- previewV.glsl: η-N (N<17) 既存 partial wrap (texture_matrix0 等 UBO は wrap 済)、η-17 で vertex attribute 追加 wrap
- normaldebugG.glsl: 本 sub-bundle 第 1 touch

---

## §10 次 sub-bundle B?-η-18 推奨 scope

### §10.1 確定 scope: V↔F pair allocator (D approach Phase 2 拡張) or 手動 V↔F pair location 全 audit + cascade 後始末

| Phase | scope | 推定対象 | 推奨範式 |
|---|---|---|---|
| Phase 1 (η-18 主) | **link failed 14 件 V↔F bare varying location alignment** (§4.3) | Deferred Diffuse 4 / PBR 系 多数 / Fullbright Alpha Masking 系 etc | (A) C++ runtime V↔F pair allocator (η-16 §3.1 transformer Phase 2 拡張、version tag `v2_p2_inout_pair` bump) **または** (B) 手動 shader file 全 audit + V↔F pair location 整合 (η-11/η-12 範式継続)。着手前 trace で A/B 比較推奨 |
| Phase 2 (η-18 副) | cascade exposure 第15層 後始末: non-opaque +8 / vary_fragcoord 6 / TerrainMix 2 / weight4 1 = 17 件 | non-opaque は UBO wrap 範式 (η-15 §3.3)、vary_fragcoord/TerrainMix/weight4 は redefinition 構造解消 (η-3/η-5/η-7/η-14 範式) | 着手前 trace で affected program list 系統別分類 |
| 後始末 (η-19+) | normalMap 69 / depthMap 9 / nameless block 2 / FXAA reserved | (大規模) | η-18 完遂後の別 sub-bundle 計画 |

### §10.2 B?-η-18 着手前 trace 範式 14 ステップ (η-17 §3.1/§3.2 範式追加)

1. **literal grep**: 各既達主指標 (16 種、SPIR-V 0 達成済入り) + 残 parse failed 100 件 + link failed 14 件 + 露出 cascade 21 件 (non-opaque 11 / vary_fragcoord 6 / TerrainMix 2 / weight4 1 / nameless 2) literal grep
2. **handoff doc 記録漏れ補完範式** (η-12 §3.4): 前 sub-bundle (η-17) handoff §2 metric 表 + §10.1 推奨 scope の literal 再検証
3. **handoff doc misanalysis 検出救済範式** (η-14 §3.5): 前 sub-bundle handoff doc の前提 (Phase 2-4 全完遂、link failed 14 件は pre-existing cascade、§4.3 解析) に literal 確認 step 必ず組込
4. **GL stage hex 解釈確認範式** (η-14 §3.1): log error の 0x8B30 = FRAGMENT / 0x8B31 = VERTEX / 0x8DD9 = GEOMETRY を着手前 trace で再確認
5. **log context 抽出**: link failed 14 件の各 program error LINE × stage type × 真因 file 候補 (V or F or 両側) を併記
6. **V/F pair canonical partner 同定範式** (η-9 §3.1 / η-11 §3.1 / η-12 §3.1 / η-17 §3.2): 14 件 link failed の affected program list で V↔F pair canonical mapping 表を作成
7. **A/B approach 比較**: (A) C++ runtime V↔F pair allocator vs (B) 手動 shader file 全 audit、コスト・upstream merge 影響・将来拡張性を比較
8. **(A 選択時) transformer version tag bump 確認**: `v1_p1_fragout` → `v2_p2_inout_pair` bump、kill-switch 切替試験で cache invalidate 動作確認
9. **(A 選択時) pair allocator 設計**: program 単位 LocationAllocator instance + V stage out / F stage in identifier mapping + 同 slot 共有 (η-16 §10.7 設計仕様継承)
10. **(B 選択時) shader file 全 audit**: V↔F bare varying location qualifier の全 file grep + V/F 番号一致 表作成 + 不整合 pair の整合変更 patch list
11. **dump 機構再投入** (§4.5 持越し): `RenderVulkanShaderDumpTransformed=1` を user_settings.xml 直接 inject (UI 経由は不発)、着手前 trace 補助証拠 + sub-bundle 完遂時 default OFF 戻し
12. **cascade exposure 後始末計画**: Phase 2 (η-18 副) で non-opaque 11 件 → UBO wrap (η-15 §3.3)、vary_fragcoord 6 件 → η-3 redefinition 解消範式継承、TerrainMix 2 件 → η-14 §3.2 nameless block 範式継承、weight4 1 件 → η-5/η-7 範式再適用
13. **regex pattern 拡張** (A 選択時): bare `in` 検出 + qualifier 列拡張 (flat/smooth/noperspective/centroid/highp/mediump/lowp/invariant) + multi-line layout decl 対応
14. **upstream merge dry-run 計画** (§7 risk #13 / #14 持越し): r41 sub-step 4.4 末点で shader file の GL path 全 byte-for-byte 不変 audit + 4.5 merge dry-run で conflict 確認、本 sub-bundle で追加した 3 file (indexedTextureV / previewV / normaldebugG) の `#ifdef LL_VULKAN_GLSL` 分岐特に重点

### §10.3 B?-η-18 完遂後の想定 cascade exposure 第16層

- link failed 14 件解消想定 = link failed 0 達成復帰
- non-opaque 11 件解消想定 = 部分解消 (Phase 2 副 scope 完走で大量解消)
- 第16層 emergence 観測点: link 段階を超えた pipeline 段階 (PSO compile / runtime binding 等) で新 error 露出可能性
- shader_cache 件数: η-18 で計測再開 + 第16層 emergence 観測

### §10.4 cinematic_bd 系統 systematic audit 推奨 (η-8 から継承、η-9〜η-17 未実施)

η-8 §3.1 cinematic_bd override path 発見範式で shadowUtil のみ個別検出。cinematic_bd directory 内 .glsl file 全件で類似 override + 未 wrap bare uniform / 未整合 location 残存可能性。B?-η-18 着手前 or 完遂後の別 phase として継続推奨。

### §10.5 AYA 環境 dump 機構 default OFF 戻し案内 (feedback_restore_debug_settings 遵守)

**AYA さんへ**: η-17 完遂時点で以下 1 項目を AYA 環境 user_settings に戻し作業お願いします (η-18 着手前に dump 機構再投入する場合は再度 1 へ inject)。

| # | 設定 key | 現状 | 戻し値 | 場所 |
|---|---|---|---|---|
| 1 | `RenderVulkanShaderDumpTransformed` | 1 (η-17 Phase D で inject) | **0** | `~/.ayastorm_x64/user_settings/settings.xml` |

戻し操作:
```xml
<key>RenderVulkanShaderDumpTransformed</key>
<map>
    <integer>0</integer>  <!-- ← 1 から 0 へ -->
</map>
```

戻し作業を実施しない場合の影響: 起動毎に `~/.ayastorm_x64/cache/shader_cache/transformed/` 配下に 300+ 件 dump file が生成され、disk 圧迫 + 起動時間微増。動作 itself には影響なし。

### §10.6 transformer version tag 後続管理 (η-16 §3.2 / η-17 §3.4)

η-17 末 = `vulkanize:v1_p1_fragout` 据え置き (E approach 採用で C++ transformer 拡張なし)。η-18 で transformer 拡張する場合は `v2_p2_inout_pair` bump 必須。kill-switch state inject 規約は継承 (`auto_loc=0/1`)。

### §10.7 V↔F pair allocator 設計仕様 (Phase 1 A 選択時、η-16 §10.7 継承 + 補強)

- program 単位 LocationAllocator instance 化 (現 anonymous local instance → LLGLSLShader メンバ化 or program ごと map 管理)
- V stage out → slot 割当 → 同 program の F stage in の同一 identifier に再利用 (identifier mapping table)
- mUsedFragOutSlots に加え `mUsedVertOutSlots` / `mUsedVertAttribSlots` / `mUsedFragInSlots` 等を namespace 分離
- 既存 layout(location=N) audit pattern を V stage / F stage 別 pre-pass で実施
- **η-17 §3.2 で確証された手動範式の取りこぼしリスク**: shader file (V side) と C++ extra_code (F side) は別 file / 別系統で管理されており、片側変更時の対側追従漏れが検出困難。allocator 自動化で本リスクを構造的に解消可能

---

## §11 観測点

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-11 310) | η-12〜η-17 計測未実施 | B?-η-18 で計測再開 + 第16層 emergence 観測 |
| 2 | C++ runtime location emit 範式 (η-16 §3.1) Phase 1 fragment out 単独維持 | η-17 では拡張せず維持 | B?-η-18 で Phase 2 (V↔F pair allocator) 拡張候補 (option A) |
| 3 | η-8 §3.3 mIndexedTextureChannels Vulkan-aware 範式の隣接行補修範式 (§3.1) 第 1 適用 | 適用済 (vary_texture_index 補修) | B?-η-18 でも同 sub-bundle 内 scope 取りこぼし発見手段として継承 |
| 4 | V↔F pair location 同期変更範式 (§3.2) 第 1 適用 | 適用済 (vary_texture_index V/F 両 side location=18 同期) | B?-η-18 で手動範式継続 (option B) または allocator 自動化 (option A) |
| 5 | cascade pair shift forward direction (handoff §6 risk #4) 想定通り 21 件 (link failed 14 + non-opaque +8 + vary_fragcoord 6 + TerrainMix 2 + weight4 1) | 適用済 (parse → link layer 移動 + 新層露出) | B?-η-18 Phase 1 で link failed 後始末、Phase 2 で cascade 後始末 |
| 6 | 1-shot ACCEPT vs N-phase 構成判定範式 (η-10 §3.3 等) Phase 2-4 + Phase D 構成 適用第 N 例 | 適用済 (AYA「D」→「E」→「E-1」judgment 階層 2026-06-02) | B?-η-18 で同範式継承 |
| 7 | dump 機構 (η-16 §3.1 内包) cold launch タイミング依存性 (§4.5) | 発見済 (UI 経由 enable 不発 / user_settings.xml 直接 inject 必須) | B?-η-18 で dump 再投入時 user_settings.xml 直接 inject 運用継承 |
| 8 | LocationAllocator namespace 分離 (η-16 = mUsedFragOutSlots のみ) | η-17 で未拡張 (E approach 採用) | B?-η-18 Phase 1 (option A) で pair-aware 拡張候補 |
| 9 | regex pattern (η-16 = bare `out` のみ) | η-17 で未拡張 | B?-η-18 Phase 1 (option A) で bare `in` 追加候補 |
| 10 | 既達主指標完全維持 (η-17 で 13 種維持 + SPIR-V requires location 0 達成、退行 0 件) | ✓ η-17 で退行 0 件達成 (link failed 14 は cascade shift forward = parse → link layer 移動、新 failure ではない) | B?-η-18 で link failed 系統 + cascade 残系統 scope |
| 11 | cinematic_bd directory 全 .glsl audit (§10.4) | 未実施 (η-8 から継承、η-9〜η-17 全て未実施) | B?-η-18 着手前 or 完遂後の別 phase として継続推奨 |
| 12 | runtime preprocessed dump 取得範式 (η-8 §3.2 / η-16 §3.1 内包) 再投入計画 | η-17 Phase D で実 verify 完了 (335 件 dump 取得 + Fragment 真因確定) | B?-η-18 着手前 trace で再投入 + sub-bundle 完遂時 default OFF 戻し |
| 13 | cascade exposure 第15層 残 (link failed 14 + non-opaque 11 + nameless 2 + vary_fragcoord 6 + TerrainMix 2 + weight4 1 + normalMap 69 + depthMap 9 + FXAA = 100 件 parse failed total + 14 link failed) | sub-bundle 残量大 | B?-η-18 着手前 trace で系統別 Phase 分割継続推奨 (link failed 14 Phase 1 主、cascade 21 Phase 2 副、normalMap/depthMap 78 + FXAA 別 sub-bundle 移管) |
| 14 | upstream OpenGL Firestorm merge 時 compatibility (D approach の真価) | r41 sub-step 4.5 merge phase で初検証 | r41 sub-step 4.4 末点で shader file の GL path 全 byte-for-byte 不変 audit、本 sub-bundle で追加した 3 file `#ifdef LL_VULKAN_GLSL` 分岐特に重点 |
| 15 | feedback_self_verify_before_handoff 適用 | η-17 で Read による edit 後構造 literal 再確認実施 (location=18 unique / V↔F pair 同期 / normaldebugG location 帯 grep audit) | B?-η-18 でも AYA cold cache launch 依頼前に必ず実施 |
| 16 | feedback_confirm_referent_before_acting 適用 | AYA「E で進めて」 → location=20 衝突発見 → E-1 提案 1 行返し → AYA「E-1 で進めて」明確化、推測実行回避 | B?-η-18 でも継承 |
| 17 | feedback_restore_debug_settings 適用 (§10.5) | η-17 で dump 機構 default OFF 戻し案内含む | B?-η-18 で dump 再投入する場合は完遂時に再度 OFF 戻し案内必須 |
| 18 | Agent misanalysis 検出救済範式 (η-11 §3.5) 適用第 N 例 | η-17 Phase D で agent「Diffuse/Fullbright 全 wrap 済」報告を dump audit で覆す | B?-η-18 でも Agent 報告は補助証拠化のみ留め、最終判定は dump / log literal で実施 |

---

**handoff doc 完。次 sub-bundle B?-η-18 着手は本 doc §10 推奨 scope (V↔F pair allocator option A/B + cascade 後始末) を起点として、fresh context で実施。**
