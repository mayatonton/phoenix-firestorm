# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-14 完遂 handoff

**status**: B?-η-14 完遂 → 次 sub-bundle B?-η-15 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: (η-12 patch `bf9164ce34` 範式継承、AYA「commit して」明示指示下 commit 予定 2026-06-02)
**handoff doc commit**: 本 doc (η-12-complete `7c1762d214` 範式継承、別 commit 予定)
**勤続範式継承**: B?-η-13-reverted (no commit) / B?-η-12-complete `7c1762d214` / B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-14 scope = **η-12 末残既達退行 3 件 (nameless block 2 + undeclared 1) 構造的解消** (η-13 retreat REVERT 後 fresh attempt、AYA「推奨案でやってください」judgment 確定 2026-06-02、Path G + G-β 採用)。

- **Phase 単一 (1-shot ACCEPT)**: **Path G** (`FrameAtmosphere_Skybox` UBO **削除**、Lighting + AtmoExtraUBO_Legacy で全 member 解決) + **Path G-β** (cloudsF.glsl に `CloudsVParamUBO_Legacy` guard 付き複製 = `cloud_scale` 提供)
  - 3 file (skyV / cloudsV / cloudsF) shader-only edit、C++ touch 0
  - canonical = `atmosphericsHelpersV.glsl` Lighting (set=0 binding=2、auto-attach 経路) + `atmosphericsFuncs.glsl` AtmoExtraUBO_Legacy (set=3 binding=0)
  - skyV/cloudsV main() touch 0 (auto-attach helper UBO で同名 member 解決、η-3 §3.2 per-group rename 範式の **超越解**)
- **Path C-2 (η-13 doc 提案) 不採用根拠 確定**: η-12 baseline log の **GL stage hex 解釈訂正** (η-13 §7.3 misanalysis) で error stage 真因が skyV **VERTEX** (0x8B31) と判明 = Path C-2 「skyF/EnvMap に Skybox 追加」前提崩壊
- **Path G 採用根拠**: skyV/cloudsV main() の参照 member は **全て Lighting (helpersV/funcs) + AtmoExtra (funcs) で賄える**、Skybox 単独 member は `haze_horizon` (AtmoExtra にあり) + `gamma` (元から未使用) のみ = Skybox 宣言自体が不要

**計**: **3 file (skyV -27 / cloudsV -27 / cloudsF +14)** shader file のみ、C++ touch 0

**主指標達成** (vs η-12 末 baseline = log `AYAstorm.old`):
- `nameless block ... global scope` 2 → **0** ✓ **-2 / 100% 完全達成**
- `undeclared identifier` 1 → **0** ✓ **-1 / 100% 完全達成**
- **不採用 Path C-2 ではなく Path G + G-β** で η-12 末退行 3 系統中 **2 系統完全解消**

**boost cascade 解消** (η-14 で副次効果、η-12 末 unique parse-failed program-stage 単位):
1. Environment Map Program (FRAG) ★ target
2. Deferred Windlight Sky Shader (FRAG) ★ target
3. Deferred Windlight Cloud Program (FRAG) ★ target
4. Deferred Windlight Sun Program (FRAG) cascade
5. Deferred Windlight Moon Program (FRAG) cascade
6. Avatar Eyeball Program (FRAG) cascade
7. Avatar Shader (FRAG) cascade
8. Deferred Avatar Eyes Shader (FRAG) cascade
9. No color alpha mask Shader (FRAG) cascade
10. Reflection Mip Shader (FRAG) cascade
11. Reflection Probe Display Shader (FRAG) cascade

**stage shift (η-14 で FRAG → VERTEX 露出、η-15+ 移管)**:
- Underwater Shader: FRAG (0x8B30) → VERTEX (0x8B31, L1099 non-opaque) shift
- Deferred Terrain Shader: FRAG (0x8B30) → VERTEX (0x8B31) shift

**新規 regression**: **0 件** ✓

**他既達主指標完全維持** (η-14 末): `overlapping use of location` 0 / `Layout location qualifier` 0 / `'binding'` 0 / `Cannot reuse block name` 0 / `'weight4'/`weight'` 0 / `Link failed` 0 / `'size'` 0 / `GBufferInfo` 0 / `'normalMap'` 69 維持 / `'depthMap'` 9 維持 / `SPIR-V requires location` 41 維持

**残退行** (η-15+ 移管): `non-opaque uniforms outside a block` 6 件 (内訳変動: Underwater VERTEX L1099 stage shift で露出、Deferred Terrain VERTEX 同様)

**Phase 構成の特徴**: η-12 cascade pair shift 順方向 ACCEPT で移管された退行 3 件のうち **2 系統 (nameless 2 + undeclared 1)** を **構造的に消す** = η-11 §3.4 cascade pair shift 範式の **逆方向 (Y count 削減で X 達成)** 第 1 例。本 sub-bundle 特徴 = **handoff doc 着手前再検証範式 (η-10 §3.1 / η-11 §3.2 / η-12 §3.4)** が **η-13 misanalysis 検出に直接効いた** 実証ケース = GL stage hex 解釈訂正で Path C-2 前提崩壊 → Path G 新規発見へ。

---

## §2 完遂結果 metric (vs B?-η-12 末 baseline log `AYAstorm.old`)

| metric | η-12 末 baseline | η-14 (verify) | Δ vs η-12 | 判定 |
|---|---|---|---|---|
| **nameless block ... global scope** | **2** | **0** | **-2** | ✓ **100% 完全達成** |
| **undeclared identifier** | **1** | **0** | **-1** | ✓ **100% 完全達成** |
| non-opaque uniforms outside a block | 6 | 6 | ±0 | △ 内訳変動 (Underwater FRAG → VERTEX stage shift、η-15+ 移管) |
| SPIR-V requires location | 41 | 41 | ±0 | (η-15+ 移管継続) |
| 'normalMap' : redefinition | 69 | 69 | ±0 | (η-15+ 移管継続) |
| 'depthMap' : redefinition | 9 | 9 | ±0 | (η-15+ 移管継続) |
| overlapping use of location | 0 | 0 | ±0 | ✓ η-12 達成維持 |
| Layout location qualifier | 0 | 0 | ±0 | ✓ η-10 達成維持 |
| 'binding' | 0 | 0 | ±0 | ✓ η-8 達成維持 |
| Cannot reuse block name | 0 | 0 | ±0 | ✓ η-1 達成維持 |
| 'weight4'/'weight' redefinition | 0 | 0 | ±0 | ✓ η-5/η-7 達成維持 |
| Link failed | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| 'size' undeclared | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| parse failed (count) | 130 | **127** | **-3** | ✓ net 縮小 (主指標 3 件解消の直接効果) |
| parse failed (unique program-stage diff) | (baseline) | -13 / +2 stage shift | net -11 | ✓ cascade 大幅縮小 (Sun/Moon/Avatar/Probe/Reflection 等 boost) |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | ±0 | ✓ |
| Goodbye | 1 | 1 | ±0 | ✓ clean shutdown |
| Vulkan device destroyed | 1 | 1 | ±0 | ✓ clean shutdown |

**主指標 2 系統 (nameless 2 + undeclared 1) 100% 完全達成 + 12 種既達主指標完全維持 + 新規 regression 0 件 + parse failed unique cascade 11 件 net 縮小** = η-14 主 scope 完全達成、Underwater/Deferred Terrain stage shift 2 件 + non-opaque 既存 4 件 = **B?-η-15 移管**。

---

## §3 設計範式

### §3.1 新規 設計範式: GL stage hex 解釈訂正範式 (η-13 misanalysis 検出 + 着手前 trace 救済)

**概要**: η-12 baseline log および η-13 handoff doc §7.3 で **GL stage hex 番号の解釈が逆** に記載されていた問題を、η-14 着手前 trace で **literal 検証** により検出 → Path C-2 前提崩壊 → Path G 新規発見へ繋げた範式。

**訂正前後の対応**:

| GL stage hex | 訂正前 (η-13 doc) | 訂正後 (η-14 確定) |
|---|---|---|
| `0x8B30` | VERTEX (誤) | **FRAGMENT** (正、`GL_FRAGMENT_SHADER`) |
| `0x8B31` | FRAGMENT (誤) | **VERTEX** (正、`GL_VERTEX_SHADER`) |

**Detection 手順** (3 step):
1. **GL header literal 確認**: `GL_VERTEX_SHADER` および `GL_FRAGMENT_SHADER` の hex 値を `/usr/include/GL/glext.h` 等で literal 確認
2. **log error line 真因 stage 再判定**: 各 parse failure の stage hex から実 stage を再特定、source file 候補 (V または F) を絞り込み
3. **当該 source file 内 error line 周辺の declaration / member 重複** を直接 grep 検証 → root cause 真因確定

**η-14 適用**: η-12 末 unique parse failure 4 件のうち:
- EnvMap (0x8B31 = **VERTEX**): 真因 = skyV (not skyF) の Skybox vs Lighting 衝突
- WLSky (0x8B31 = **VERTEX**): 真因 = skyV の同衝突
- WLCloud (0x8B30 = **FRAGMENT**): 真因 = cloudsF の `cloud_scale` 未宣言 (CloudsFParamUBO_Legacy 欠落)
- Underwater (0x8B31 = **VERTEX**): 真因 = L1099 non-opaque、η-15+ 移管

→ η-13 §7.3 Path C-2 「skyF/EnvMap に Skybox 追加」は **stage 誤判定** に基づく前提、Path G「skyV/cloudsV の Skybox 宣言削除」が真の解。

**handoff doc 記録価値**: 後続 sub-bundle で stage 同定時、必ず GL header literal 確認 step を着手前 trace に組み込む = stage 誤判定 misanalysis を再発させない範式。

### §3.2 新規 設計範式: nameless block 重複衝突 構造的解消範式 (auto-attach helper 委譲)

**概要**: nameless interface block の member が **global scope** で重複する場合、片方の宣言を **削除** し auto-attach helper 経由の他方に委譲する範式。η-4 §3.1 nameless block × padding 範式の **超越解**。

**従来 (η-4 §3.1 範式)**: nameless block 同 member が複数 file に重複宣言される場合、padding 完全一致 (struct memory layout 一致) + `#ifndef X_DEFINED` guard wrap で de-dup。同 binding 占有の異 block (Skybox vs Lighting) は **per-group rename** (η-3 §3.2) で回避。

**η-14 新範式**: per-group rename しても **member 名 global scope** で衝突する場合 (nameless block の member は global scope に持ち上がる GLSL 仕様)、**片方の block declaration 自体を削除** し、auto-attach helper 経由で確保される他方 block に member 解決を委譲する。

**Detection 手順** (4 step):
1. **重複衝突 block 列挙**: 主 shader file (e.g. skyV) と auto-attach helper (e.g. atmosphericsHelpersV) の同 binding nameless block を grep で列挙
2. **member 重複表作成**: 両 block の全 member を一覧化、重複 member を抽出
3. **main() 参照 member 全件解決確認**: 主 shader の main() で参照される member が、削除候補でない他方 block で **全て解決可能** か検証
4. **欠落 member の代替 source 同定**: 主 shader 単独 member が主 shader 削除で欠落する場合、他 helper (e.g. AtmoExtraUBO_Legacy in atmosphericsFuncs) 経由で別途確保されているか確認 (η-14 では haze_horizon は AtmoExtra 経由、gamma は未使用)

**η-14 適用**: skyV の `FrameAtmosphere_Skybox` (22-member) を削除、`atmosphericsHelpersV.glsl` 経由の Lighting (20-member) + `atmosphericsFuncs.glsl` 経由の AtmoExtraUBO_Legacy (10-member) で全 member 解決:

| member | Skybox (削除) | Lighting (helpersV/funcs) | AtmoExtra (funcs) |
|---|---|---|---|
| sunlight_color / blue_horizon / ambient_color / haze_density / max_y / glow / blue_density / density_multiplier 他 14 件 | ○ | ○ | - |
| haze_horizon | ○ | - | ○ (代替) |
| gamma | ○ (未使用) | - | - (drop OK) |

→ Skybox 削除で main() touch 0、構造的に nameless block 衝突解消。

**handoff doc 記録価値**: 後続 sub-bundle で **同 binding 同 member 群** の重複衝突を発見した際、per-group rename (η-3 §3.2) ではなく **片方削除 + auto-attach 委譲** を第一候補化する範式。前提条件 = 削除候補 block の全 member が **他 helper 経由で解決可能**。

### §3.3 新規 設計範式: 同 UBO 別 stage 複製範式 (CloudsVParamUBO_Legacy fragment 複製)

**概要**: vertex stage で declare された UBO を fragment stage 側でも参照する必要がある場合、**同 layout (set/binding/std140/member 順) を guard 付きで複製** する範式。

**Detection 手順** (3 step):
1. **fragment 側未宣言 member 検出**: fragment shader main() の参照 member のうち、fragment 側 UBO declaration に **無い** member を grep で抽出
2. **vertex 側 UBO で member 確保確認**: 該当 member が vertex 側 UBO で declare されているか grep で確認
3. **fragment 側 guard 付き複製**: vertex 側 UBO の declaration を **完全同 layout + guard `<NAME>_DEFINED`** で fragment shader に複製。C++ descriptor set 側は pipeline 単位で両 stage 参照可能なため struct 改修不要

**η-14 適用**: cloudsF.main() L127/L132/L133 が `cloud_scale` 参照、cloudsF の `CloudsFParamUBO_Legacy` (set=3 binding=4) は `cloud_scale` 未収載 → cloudsV の `CloudsVParamUBO_Legacy` (set=3 binding=3) を fragment 側にも guard `CLOUDS_V_PARAM_UBO_LEGACY_DEFINED` 付きで複製 = `cloud_scale` 解決。

**handoff doc 記録価値**: 後続 sub-bundle で fragment shader が vertex stage UBO member を参照する必要がある場合の **shader-only 解** を確立 (charter §3 #1 遵守、C++ touch 不要)。

### §3.4 設計範式継承: η-11 §3.4 cascade pair shift 逆方向 既達退行解消範式 第 1 例

**概要**: η-11 §3.4 で確立された cascade pair shift 順方向 (X 解消 → Y 退行) ACCEPT 判定範式の **逆方向 (Y 削減 → X 達成)** 適用第 1 例。

**η-14 適用**:

| Detection step | 結果 | 判定 |
|---|---|---|
| 1. η-12 末退行 Y (η-14 開始時の baseline) | 3 系統 4 件 (nameless 2 + undeclared 1 + non-opaque 増分 1) | - |
| 2. η-14 解消 ΔY (主指標削減) | 2 系統 3 件 (nameless 2 + undeclared 1) | - |
| 3. η-14 新規退行 X' | 0 件 | - |
| 4. parse failed unique 経由 net cascade 縮小 | 11 件 net (-13 + stage shift +2) | - |
| **総合判定** | **Y 削減 + X' 0 + cascade 縮小** | **完全 ACCEPT (退行なし、boost 副次効果)** |

**handoff doc 記録価値**: η-11 §3.4 の cascade pair shift 範式は **両方向に対称** であることを実証 = 順方向 (X→Y) ACCEPT 判定範式が逆方向 (Y→X 達成) にも自然延長して適用可能。

### §3.5 設計範式継承: η-10 §3.1 / η-11 §3.2 / η-12 §3.4 handoff doc 着手前再検証範式の **misanalysis 検出救済範式 第 1 例**

**概要**: handoff doc 着手前再検証範式が **前 sub-bundle の analytical error** を検出する救済範式として機能した第 1 例。

**η-14 適用**:
- η-13 handoff doc §7.3 = 「Path C-2 (skyF/EnvMap に Skybox 追加)」を η-14 着手前 trace 必須項目として記載
- η-14 着手前 trace step 1 (literal grep): η-12 baseline log の error line stage hex を再検証
- η-14 着手前 trace step 3 (handoff doc canonical 記載再検証): GL stage hex 解釈の literal 確認 → 訂正発見
- → Path C-2 前提崩壊検出 → Path G 新規発見

**handoff doc 記録価値**: 後続 sub-bundle で handoff doc 推奨 scope の前提 (stage 同定、root cause 推定等) に **literal 確認 step** を必ず組み込む範式。前 sub-bundle handoff doc は **絶対真理ではなく被検証対象** として扱う = feedback_doubt_self_first 範式の handoff doc 適用形 (η-12 §3.4 と同方向、misanalysis 検出に拡張)。

### §3.6 設計範式継承表 (η-1〜η-13 全件継承)

| η-N | 範式 | η-14 適用状況 |
|---|---|---|
| η-1 §3.1 | FrameViewProj guard wrap (set=0/binding=0) | η-14 適用外 (declare 削除側) |
| η-1 §3.2 | cascade pair hypothesis (主指標 -X ↔ cascade +Y) | η-14 §3.4 で **逆方向第 1 例** |
| η-1 §3.3 | Agent 並列 disjoint scope | η-14 適用外 (直接 grep 同定で済、Agent 投入なし) |
| η-2 (a) §3.2 | UBO body member 順序 std140 完全一致 | η-14 §3.3 同 UBO 別 stage 複製範式で **完全 layout 一致** 担保 |
| η-3 §3.1/§3.2 | Link failed root cause UBO body 差異解消 + per-group rename | η-14 §3.2 で **per-group rename の超越解** (片方削除) 提示 |
| η-4 §3.1/§3.2/§3.4 | nameless block × padding / cascade pair / UBO set=3 帯 | η-14 §3.2 で **nameless block 重複衝突 構造的解消範式 (auto-attach helper 委譲)** に拡張 |
| η-5 §3.1〜§3.4 | multi-root-cause / 5th-level refinement / WEIGHT_LOCATION guard 等 | η-14 適用外 |
| η-6 §3.1〜§3.5 | multi-cluster pilot / Agent 誤判定 / nameless block × member 第 3 例 等 | η-14 適用外 (Agent 投入なし) |
| η-7 §3.1〜§3.5 | bvec2→uvec2 / program name collision / 5th-level cascade / nameless block × member 第 4 例 / cascade pair 順方向前駆 | η-14 §3.4 で cascade pair 逆方向第 1 例継承 |
| η-8 §3.1〜§3.4 | cinematic_bd override / runtime preprocessed dump / mIndexedTextureChannels Vulkan-aware / UBO wrap | η-14 では runtime dump 未投入 (static trace で同定可能) |
| η-9 §3.1 | V/F pair canonical partner 同定範式 | η-14 適用外 (location qualifier 系でなく nameless block 系) |
| η-9 §3.2 | Phase 1 falsification + 完全 revert + Phase 2 軌道修正 | η-14 では Phase 1 即 ACCEPT (1-shot 完結) |
| η-9 §3.3/§3.4 | falsification_as_progress / admit_unknown | η-14 §3.5 で η-13 misanalysis 検出として継承 |
| η-10 §3.1 | handoff doc canonical 記載 着手前再検証範式 | η-14 §3.5 で **misanalysis 検出救済範式 第 1 例** に拡張 |
| η-10 §3.2 | V/F pair canonical partner 第 2 例 | η-14 適用外 |
| η-10 §3.3 | 1-shot ACCEPT vs 2-phase 構成判定範式 | η-14 で 1-shot ACCEPT 適用第 4 例 |
| η-11 §3.1 | V/F pair canonical partner 第 3 例 (auto-attach helper 経路) | η-14 §3.2 で **auto-attach helper 委譲 (片方削除)** に拡張 |
| η-11 §3.2 | handoff doc canonical 記載 着手前再検証範式 第 2 例 | η-14 §3.5 で継承 |
| η-11 §3.3 | 1-shot ACCEPT vs 2-phase 構成判定範式 第 2 例 | η-14 で第 4 例継承 |
| η-11 §3.4 | cascade pair shift 順方向 既達退行 ACCEPT 判定範式 | η-14 §3.4 で **逆方向第 1 例** (Y 削減 → X 達成) |
| η-11 §3.5 | Agent 報告検証 step V/F pair 同定範式違反検出範式 | η-14 適用外 (Agent 投入なし) |
| η-12 §3.1 | V/F pair canonical partner 第 4 例 | η-14 適用外 |
| η-12 §3.2 | cascade pair shift Y = X 境界条件範式拡張 | η-14 §3.4 で逆方向に転用 |
| η-12 §3.3 | 1-shot ACCEPT vs 2-phase 構成判定範式 第 3 例 | η-14 で第 4 例継承 |
| η-12 §3.4 | handoff doc 記録漏れ補完範式 | η-14 §3.5 で **misanalysis 検出救済範式** に拡張 |
| η-13 §3.5 | retreat-revert 範式 (Y ≫ X で完全 revert + no commit + -reverted suffix) | η-14 では fresh attempt 成功で適用外、η-13 retreat 結果を Path C-2 不採用根拠として継承 |
| η-13 §3.6 | 同 binding 多 block member 重複衝突 diagnostic | η-14 §3.2 で **構造的解消範式 (片方削除)** に発展 |
| η-13 §3.7 | 4-Path 比較範式 (B/C-1/C-2/C-3) | η-14 で **Path G 新規追加 + C-2 不採用根拠確定** (stage 真因訂正) |

**feedback memory 10 件全件適用確認**:
- feedback_doubt_self_first: η-14 §3.5 適用 (η-13 handoff doc §7.3 を絶対真理視せず literal 検証 → GL stage hex 訂正発見)
- feedback_root_cause_not_dump: Skybox 削除で構造的に nameless block 衝突解消 (workaround でなく根本対処)
- feedback_shader_only_fast_iterate: shader cp + cache clear のみ (autobuild 不要、C++ touch 0)
- feedback_no_auto_commit: AYA「commit して」明示指示後に commit (本 doc 起草時点で未 commit)
- feedback_no_claude_coauthor: commit message に Co-Authored-By 不在予定
- feedback_one_step_at_a_time: 着手前 trace → Path 比較 + Path G 新規提示 → AYA judgment「推奨案でやってください」→ patch → deploy → verify → AYA「OK」→ handoff doc 起草 を順次実施
- feedback_proactive_handoff: 本 handoff doc 起草
- feedback_no_scope_shrink: η-14 scope (3 系統中 2 系統 = nameless 2 + undeclared 1) は AYA 明示指示 (η-12 §10.1 推奨 + 本 sub-bundle AYA「推奨案で」judgment) 下確定 = scope shrink でなく **構造的 scope 確定** (non-opaque 1 件 stage shift は η-15+ 移管、Underwater L1099 は η-12 時点で既に η-15+ 移管予定)
- feedback_admit_unknown: η-14 では仮説 1 連続外れず (η-13 misanalysis 検出 → 即 Path G 発見 → 検証 PASS)
- feedback_falsification_as_progress: η-13 Path B 失敗履歴を Path C-2 不採用根拠 (stage 誤判定起源) の補強として活用、retreat-revert (η-13 §3.5) → fresh attempt (η-14) の **両周回学習** 実証

---

## §4 cold cache launch verify 詳細

### §4.1 verify cycle 構成 (1 cycle = 1-shot ACCEPT)

**Cycle 1 (Phase 1 verify = ACCEPT)**:
- 操作: 3 file edit → shader-only fast-iterate cp 3 file → shader_cache clear (0 件 verify) → AYA cold cache launch (2026-06-01T23:56:01Z 起動) → shutdown
- 結果:
  - `nameless block ... global scope` 2 → **0** (-2 ✓ 100% 完全達成)
  - `undeclared identifier` 1 → **0** (-1 ✓ 100% 完全達成)
  - `non-opaque uniforms outside a block` 6 → 6 (±0、内訳変動 = Underwater FRAG → VERTEX stage shift、Deferred Terrain 同様)
  - 14 種既達主指標 0 完全維持 + normalMap 69 / depthMap 9 / SPIR-V 41 維持
  - parse failed count 130 → 127 (-3 net、主指標 3 件解消直接効果)
  - parse failed unique program-stage diff: -13 件解消 + 2 件 stage shift = **net -11 件 cascade 縮小**
  - clean shutdown (Goodbye 1 + Vulkan device destroyed 1 + FATAL/SIGSEGV/Aborted 0)
- 判定: **η-14 deliverable 確定 (1-shot ACCEPT、AYA「OK」judgment 確定 2026-06-02)**

### §4.2 主指標 metric integrity self-check

- `nameless block ... global scope` -2 = skyV/cloudsV の Skybox 宣言削除直接効果 (Lighting binding=2 と global scope member 名衝突解消)
- `undeclared identifier` -1 = cloudsF への CloudsVParamUBO_Legacy 複製で `cloud_scale` 解決
- `'location'` literal grep 0→0 (Layout match 0 + overlapping 0 + SPIR-V 41 は literal 'location' 経由なしの error category)
- parse failed 130→127 (主指標 3 件解消の count 直接効果、unique program-stage では cascade boost で 11 件 net 縮小)

### §4.3 boost cascade 解消 (副次効果、unique program-stage diff)

| # | program | stage | η-12 末 baseline | η-14 result | 原因 |
|---|---|---|---|---|---|
| 1 | Environment Map Program | FRAG (0x8B30) | ✗ | ✓ | skyV Skybox 削除で skyV TU 全体 parse 成功 → リンク成功 |
| 2 | Deferred Windlight Sky Shader | FRAG (0x8B30) | ✗ | ✓ | 同上 |
| 3 | Deferred Windlight Cloud Program | FRAG (0x8B30) | ✗ | ✓ | cloudsV Skybox 削除 + cloudsF CloudsVParamUBO_Legacy 複製で両 stage parse 成功 |
| 4 | Deferred Windlight Sun Program | FRAG (0x8B30) | ✗ | ✓ | sunV/sunF (skyV/F と auto-attach helper 群共有) cascade 解消 |
| 5 | Deferred Windlight Moon Program | FRAG (0x8B30) | ✗ | ✓ | moonV/moonF cascade 解消 |
| 6 | Avatar Eyeball Program | FRAG (0x8B30) | ✗ | ✓ | helper 経由の Lighting 整合改善 cascade |
| 7 | Avatar Shader | FRAG (0x8B30) | ✗ | ✓ | 同上 |
| 8 | Deferred Avatar Eyes Shader | FRAG (0x8B30) | ✗ | ✓ | 同上 |
| 9 | No color alpha mask Shader | FRAG (0x8B30) | ✗ | ✓ | helper 経由 cascade |
| 10 | Reflection Mip Shader | FRAG (0x8B30) | ✗ | ✓ | 同上 |
| 11 | Reflection Probe Display Shader | FRAG (0x8B30) | ✗ | ✓ | 同上 |

target 3 件 + cascade ボーナス 8 件 = unique net -11 件解消 (parse failed count -3 と整合: 同 program-stage が複数 cycle で重複 log されるため)。

### §4.4 stage shift 詳細 (η-15+ 移管)

| # | program | η-12 末 stage | η-14 stage | 原因 |
|---|---|---|---|---|
| 1 | Underwater Shader | FRAG (0x8B30) | VERTEX (0x8B31) | FRAG 死で覆い隠されていた VERTEX L1099 non-opaque uniforms 露出 |
| 2 | Deferred Terrain Shader | FRAG (0x8B30) | VERTEX (0x8B31) | 同様 (FRAG 死カスケード解消で VERTEX 露出) |

これは regression でなく **progress** (FRAG 死で隠れていた VERTEX errors が露出 = error 階層深化)。η-15+ で対処。

### §4.5 shutdown clean verify

- Cycle 1 で:
  - `Goodbye!` 1 件 + `Vulkan device destroyed` 1 件
  - FATAL/SIGSEGV/Aborted 0/0/0

---

## §5 self-verify 18 項目 all green

| # | 項目 | 状態 |
|---|---|---|
| 1 | charter §3 #1 GL path byte-for-byte 不変担保 (3 file GL `#else` path 元宣言不変、Vulkan path 内のみ edit) | ✓ |
| 2 | shader file のみ touch (C++ touch 0) | ✓ |
| 3 | 主指標 `nameless block ... global scope` 2→0 literal grep verify | ✓ |
| 4 | 主指標 `undeclared identifier` 1→0 literal grep verify | ✓ |
| 5 | 既達主指標 14 種 literal grep verify 全 0 維持 + normalMap/depthMap/SPIR-V 69/9/41 維持 | ✓ |
| 6 | parse failed count 130→127 net 縮小 verify + unique program-stage diff -11 件 cascade 縮小 verify | ✓ |
| 7 | FATAL/SIGSEGV/Aborted 0 維持 | ✓ |
| 8 | clean shutdown (Goodbye 1 + Vulkan destroy 1) | ✓ |
| 9 | GL stage hex 解釈訂正範式 §3.1 新規確立 (η-13 misanalysis 検出 + Path C-2 不採用根拠確定) | ✓ |
| 10 | nameless block 重複衝突 構造的解消範式 §3.2 新規確立 (auto-attach helper 委譲 = per-group rename 超越解) | ✓ |
| 11 | 同 UBO 別 stage 複製範式 §3.3 新規確立 (CloudsVParamUBO_Legacy fragment 複製、shader-only 解) | ✓ |
| 12 | cascade pair shift 逆方向 既達退行解消範式 §3.4 第 1 例 (Y 削減 → X 達成、η-11 §3.4 対称延長) | ✓ |
| 13 | handoff doc misanalysis 検出救済範式 §3.5 第 1 例 (η-13 §7.3 stage 誤判定検出) | ✓ |
| 14 | η-9 §3.3 falsification 履歴遵守 (η-13 Path B 失敗 retreat-revert 結果を Path C-2 不採用根拠補強として活用) | ✓ |
| 15 | new safe binding/location 番号占有なし (η-14 は declaration 削除側 + 既存 binding=3 set=3 複製のみ) | ✓ |
| 16 | feedback_no_claude_coauthor 遵守 (commit message Co-Authored-By 不在予定) | ✓ |
| 17 | feedback_no_auto_commit 遵守 (AYA「commit して」明示指示後に commit) | ✓ |
| 18 | handoff doc 別 commit (η-12-complete `7c1762d214` 範式継承) | ✓ |

---

## §6 設計範式継承表

(§3.6 と内容重複のため §3.6 参照、feedback memory 10 件全件適用確認 + prior sub-bundle 32+ 件継承 + 新規 5 件 §3.1/§3.2/§3.3/§3.4/§3.5)

---

## §7 risks

| # | risk | mitigation |
|---|---|---|
| 1 | §3.2 nameless block 重複衝突 構造的解消範式の **濫用** (任意の block を削除すると main() 参照 member が欠落) | §3.2 Detection 手順 step 3「main() 参照 member 全件解決確認」必須、削除前に他 helper 経由解決を literal 確認 |
| 2 | §3.3 同 UBO 別 stage 複製範式の **layout 不一致** (vertex 側 UBO が後で改修されたら fragment 側 複製と divergence) | guard `<NAME>_DEFINED` で de-dup 担保、後続 sub-bundle で UBO 改修時は **両 stage 同時改修** を範式化検討 |
| 3 | §3.1 GL stage hex 解釈訂正範式の **literal 確認漏れ** (header 値が後で変更される可能性は理論上ある) | GL 標準値 (0x8B30=FRAG, 0x8B31=VERT) は固定、ただし後続 sub-bundle 着手前 trace step 1 で必ず再確認 |
| 4 | gamma member 削除で **後続 sub-bundle で gamma 参照復活時の cascade** (η-14 で skyV/cloudsV 未使用確認のみ、他 sub-bundle で参照復活なら欠落) | skyV/cloudsV main() で gamma 参照復活時は AtmoExtraUBO_Legacy 拡張 or 別 helper UBO 追加で対処、Skybox 復活は per-group rename (η-3 §3.2) 戻りで非推奨 |
| 5 | Underwater / Deferred Terrain stage shift (FRAG → VERTEX) で **既存 hidden bug 露出** = η-15+ 対処時に root cause 探索負担増 | η-15 着手前 trace で stage shift 2 件の VERTEX L1099/L1146 周辺の bare uniform 列挙 (η-8 §3.4 UBO wrap 範式適用候補) |
| 6 | 残退行 non-opaque 6 件 (Underwater L1099 + 既存 5 件) η-15+ 移管で **残量持越** | η-15 着手前 trace で non-opaque 系統対処時に Underwater 含む 6 件全件まとめて UBO wrap 処理 (η-8 §3.4 範式継承) |
| 7 | SPIR-V missing 41 + normalMap 69 + depthMap 9 = 計 119 件 η-15+ 移管 = **sub-bundle 残量大** | η-15 着手前 trace で系統別 Phase 分割継続 (SPIR-V 41 件 = C++ runtime emit Vulkan-aware 化 η-8 §3.3 範式優先、normalMap/depthMap 別 sub-bundle 分割推奨) |
| 8 | C++ touch 0 維持 (η-8 で 1 例発生、η-9〜η-14 で 0) | η-15 で SPIR-V missing 41 件対処時に C++ touch 復活可能性 (η-8 §3.3 範式)、shader-only 範式の自然延長で構造的限界がある場合のみ |
| 9 | cinematic_bd directory 全 .glsl audit (η-8 から継承、η-9〜η-14 未実施) | η-15 着手前 or 完遂後の別 phase として強く推奨 (η-8 §3.1 cinematic_bd override path 発見範式の予防的展開) |
| 10 | shader_cache 件数 (η-7 305 baseline → η-11 310) η-12/η-14 計測未実施 | η-15 で shader_cache 件数計測再開 + 第14層 emergence 観測 |
| 11 | Path G で skyV/cloudsV の `FrameAtmosphere_Skybox` UBO 宣言を削除したが **C++ 側で同 UBO struct + binding 登録は維持** = 描画は問題なし (shader 側で参照しないだけ)、ただし将来 C++ 側 cleanup で UBO struct 削除する場合に skyV/cloudsV 復活への影響 | C++ 側 Skybox UBO struct + binding 削除は別 sub-bundle scope、η-14 では shader 側参照削除のみ完結、整理は η-15+ で C++ 側 Skybox UBO 用途残存確認 → 用途 0 なら C++ 側削除 |
| 12 | runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画 (η-9〜η-14 未投入) | η-15 で root cause が static trace 不能な場合に再投入 + sub-bundle 完遂時除去 |

---

## §8 commit history

| commit | sub-bundle | scope |
|---|---|---|
| (η-14 patch 予定) | B?-η-14 patch | 3 file (skyV / cloudsV / cloudsF) Path G + G-β (Skybox UBO 削除 + CloudsVParamUBO 複製) |
| (η-13 no commit) | B?-η-13 retreat-revert | Path B (haze_horizon unification) 失敗 → 完全 revert、handoff `-reverted` suffix のみ |
| `7c1762d214` | B?-η-12 handoff doc | (η-12 完遂 handoff、η-13 着手境界引継) |
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

## §9 file inventory (η-14 patch 3 file 内訳)

| # | file | 変更内容 | 行数 | 範式適用 |
|---|---|---|---|---|
| 1 | `indra/newview/app_settings/shaders/class1/deferred/skyV.glsl` | Vulkan path `FrameAtmosphere_Skybox` UBO 宣言削除 (binding=2 set=0、22-member nameless block) → η-14 marker comment 置換 | +6/-27 | §3.2 nameless block 重複衝突 構造的解消範式 (auto-attach Lighting 委譲) |
| 2 | `indra/newview/app_settings/shaders/class1/deferred/cloudsV.glsl` | 同上 (Vulkan path `FrameAtmosphere_Skybox` UBO 宣言削除) | +3/-27 | §3.2 同範式 |
| 3 | `indra/newview/app_settings/shaders/class1/deferred/cloudsF.glsl` | Vulkan path `CloudsVParamUBO_Legacy` (set=3 binding=3、4-member) guard 付き複製追加 (`CLOUDS_V_PARAM_UBO_LEGACY_DEFINED`)、cloud_scale 解決 | +14/-0 | §3.3 同 UBO 別 stage 複製範式 (cloudsV partner、shader-only) |

**計**: 3 file +23/-54 net -31、shader file のみ、C++ touch 0

**overlap file 確認**:
- skyV.glsl: η-11 + η-12 で location numeric edit 済、η-14 で第 3 touch (UBO declaration 削除) = B?-δ 範式 admission 第 3 touch
- cloudsV.glsl: 同上、η-14 第 3 touch
- cloudsF.glsl: η-11 + η-12 で location numeric edit 済、η-14 で第 3 touch (UBO 複製追加)
- atmosphericsHelpersV.glsl / atmosphericsFuncs.glsl / skyF.glsl: η-14 で参照のみ (canonical 同定)、touch 0
- llshadermgr.cpp / llglslshader.cpp / llviewershadermgr.cpp: η-14 で参照のみ (auto-attach 経路同定 + program file 列挙)、touch 0

---

## §10 次 sub-bundle B?-η-15 推奨 scope

### §10.1 確定 scope: stage shift 露出 2 件 + 既存 non-opaque 4 件 + 第10層 emergence 残全件

| metric | η-14 末件数 | 推定 root cause | 推奨対処 |
|---|---|---|---|
| **non-opaque uniforms outside a block** | **6** | Underwater VERTEX L1099 (stage shift 露出) + Deferred Terrain VERTEX (stage shift 露出) + 既存 4 件 (SMAA Blending Weights V ×4 line 1712 想定) | 専用 phase (bare uniform 探索 + UBO wrap、η-8 §3.4 範式継承) |
| SPIR-V requires location | 41 | in/out 宣言の location 番号未指定 (mass) | C++ runtime emit Vulkan-aware 化 (η-8 §3.3 範式) or 一括 wrap script |
| 'normalMap' redefinition | 69 | (η-1〜η-14 未対処、新 root cause 系統) | 専用 phase (Agent 並列 disjoint scope η-1 §3.3) |
| 'depthMap' redefinition | 9 | 同上 | 同上 |
| missing #endif / parse failed | cascade 縮小傾向 | cascade pair shift | 主指標解消後の自然減 |

### §10.2 B?-η-15 着手前 trace 範式 10 ステップ (η-14 §3.1/§3.5 範式追加)

1. **literal grep**: 各既達主指標 (16 種) の literal grep + non-opaque 6 件 + SPIR-V 41 件 + normalMap 69 + depthMap 9 件の grep
2. **handoff doc 記録漏れ補完範式** (η-12 §3.4): 前 sub-bundle (η-14) handoff §2 metric 表 + §10.1 推奨 scope の literal 再検証
3. **handoff doc misanalysis 検出救済範式** (**η-14 §3.5 新規**): 前 sub-bundle handoff doc の前提 (stage 同定、root cause 推定等) に **literal 確認 step** 必ず組込
4. **GL stage hex 解釈確認範式** (**η-14 §3.1 新規**): log error の 0x8B30 = FRAGMENT / 0x8B31 = VERTEX を着手前 trace で再確認
5. **log context 抽出**: 各 program の error LINE × 件数 + stage type 番号 + 真因 file 候補 (V or F) を併記 (η-6 §3.2 範式)
6. **V/F pair / auto-attach helper 同定** (η-9 §3.1 / η-10 §3.2 / η-11 §3.1 / η-12 §3.1 / η-14 §3.2): location 系は V/F pair、UBO 系は auto-attach helper 経路同定
7. **nameless block 重複衝突 検出 + 構造的解消可能性 verify** (**η-14 §3.2 新規**): nameless block 重複検出時、auto-attach helper 委譲 (片方削除) 可能性を main() 参照 member 全件解決確認で verify
8. **同 UBO 別 stage 複製判定** (**η-14 §3.3 新規**): fragment shader が vertex stage UBO member 参照する場合、shader-only 複製で解決可能性を guard 付き complete-layout 複製で適用
9. **Agent 並列 disjoint scope** (η-1 §3.3 範式): metric 系統別に root cause 軸で分割 (non-opaque / SPIR-V missing / normalMap / depthMap 各別 Agent)
10. **cascade pair shift 既達退行 ACCEPT 判定** (η-11 §3.4 / η-12 §3.2 / **η-14 §3.4 逆方向追加**): 順方向 (Y < X ACCEPT) + 逆方向 (Y 削減 → X 達成) + Y = X 境界条件 (AYA judgment)

### §10.3 B?-η-15 完遂後の想定 cascade exposure 第14層

- non-opaque 6 件解消想定 = parse failed -6 程度 (Underwater/Deferred Terrain VERTEX stage 復活含む)
- SPIR-V missing 41 件 → C++ runtime emit Vulkan-aware 化で大量解消想定
- normalMap / depthMap redefinition 78 件 → 別 sub-bundle 分割推奨 (B?-η-16 移管候補)
- shader_cache 件数: η-15 で 310+ 維持想定 + 第14層 emergence 観測

### §10.4 cinematic_bd 系統 systematic audit 推奨 (η-8 から継承、η-9〜η-14 未実施)

η-8 §3.1 cinematic_bd override path 発見範式で shadowUtil のみ個別検出。cinematic_bd directory 内 .glsl file 全件で類似 override + 未 wrap bare uniform / 未整合 location 残存可能性。B?-η-15 着手前 or 完遂後の別 phase として継続推奨:

1. `find indra/newview/app_settings/shaders/cinematic_bd -name "*.glsl"` で全件列挙
2. 各 file の bare uniform / sampler / `layout` 欠落 + location qualifier 不整合 grep audit
3. 該当 file を class1/class2/class3 版 patch と同 set/binding + location で wrap / 整合 (mutually exclusive 担保)

### §10.5 runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画継承

η-9〜η-14 未投入 (static trace + literal grep + handoff doc misanalysis 検出範式で同定可能だった)。η-15 で root cause が static trace 不能な場合、η-8 §3.2 範式再投入 → sub-bundle 完遂時に必ず除去。

### §10.6 nameless block 重複衝突 構造的解消可能性事前 audit 範式 (η-14 §3.2 から派生)

η-14 教訓: nameless block 衝突は per-group rename (η-3 §3.2) より **片方削除 + auto-attach 委譲** が構造的に優れる場合がある。η-15+ で nameless block 系 error 発見時:

1. **重複衝突 block 列挙**: 主 shader と auto-attach helper の同 binding nameless block を grep
2. **member 重複表 + main() 参照 member 列挙**
3. **削除候補 block 決定**: 単独 member が 0 件 or 他 helper 経由解決可能なら **削除可能**
4. **欠落 member 代替 source 同定 verify**: AtmoExtraUBO_Legacy 等の他 helper UBO で確保確認
5. **削除実行 + verify**

### §10.7 同 UBO 別 stage 複製範式 適用 audit (η-14 §3.3 から派生)

η-14 教訓: fragment shader が vertex stage UBO member 参照する場合、guard 付き complete-layout 複製で shader-only 解可能。η-15+ で類似 case 発見時:

1. **fragment 側未宣言 member 列挙**
2. **vertex 側 UBO で member 確保確認**
3. **fragment 側 guard 付き complete-layout 複製**
4. **C++ descriptor set 側無改修確認** (pipeline 単位で両 stage 参照可能)

---

## §11 観測点

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-11 310) | η-12/η-14 計測未実施 | B?-η-15 で計測再開 + 第14層 emergence 観測 |
| 2 | nameless block 重複衝突 構造的解消範式 (§3.2) 新規確立 | 適用済 (skyV/cloudsV Skybox 削除、Lighting 委譲) | B?-η-15 で nameless block 系 error 発見時に §10.6 audit 適用 |
| 3 | 同 UBO 別 stage 複製範式 (§3.3) 新規確立 | 適用済 (cloudsF に CloudsVParamUBO_Legacy 複製) | B?-η-15 で類似 case 発見時に §10.7 audit 適用 |
| 4 | GL stage hex 解釈訂正範式 (§3.1) 新規確立 | 適用済 (η-13 misanalysis 検出、Path C-2 不採用根拠確定) | B?-η-15 着手前 trace step 4 として継続適用 |
| 5 | cascade pair shift 逆方向 既達退行解消範式 (§3.4) 第 1 例 | 適用済 (Y 削減 3 件 → X 達成 nameless 2 + undeclared 1) | B?-η-15 で逆方向適用第 2 例可能性 |
| 6 | handoff doc misanalysis 検出救済範式 (§3.5) 第 1 例 | 適用済 (η-13 §7.3 stage 誤判定検出) | B?-η-15 着手前 trace step 3 として継続適用 |
| 7 | 1-shot ACCEPT vs 2-phase 構成判定範式 (η-10 §3.3 / η-11 §3.3 / η-12 §3.3) 第 4 例 | 適用済 (1-shot ACCEPT、Agent 投入なし) | B?-η-15 で着手前 trace 確定度で判定 |
| 8 | Agent 報告検証 step V/F pair 同定範式違反検出範式 (η-11 §3.5) 適用条件明示化 | η-14 では未適用 (直接 grep 同定で済) | B?-η-15 で Agent 投入時に適用 |
| 9 | set/binding allocation 連続割当 (η-8 末: 14-59 + 100-103) | η-14 で binding touch なし (削除側 + 既存 binding=3 set=3 複製のみ) | B?-η-15 で SPIR-V missing 41 件対処時に binding 触る場合は 60+ 連続割当開始 |
| 10 | C++ touch 0 維持 (η-8 で 1 例発生、η-9〜η-14 で 0) | η-14 達成 | B?-η-15 で SPIR-V missing 41 件対処時に C++ touch 復活可能性 (η-8 §3.3 範式) |
| 11 | 既達主指標完全維持 (η-14 で 14 種、退行 0 件) | ✓ η-14 で退行 0 件達成 | B?-η-15 で 14 種維持 + 残系統 scope |
| 12 | cinematic_bd directory 全 .glsl audit (§10.4) | 未実施 (η-8 から継承、η-9〜η-14 全て未実施) | B?-η-15 着手前 or 完遂後の別 phase として継続推奨 |
| 13 | runtime preprocessed dump 取得範式 (η-8 §3.2) 再投入計画 | η-9〜η-14 未投入 | B?-η-15 で root cause が static trace 不能な場合に再投入 + sub-bundle 完遂時除去 |
| 14 | C++ 側 Skybox UBO struct + binding 用途残存確認 (η-14 §7 #11) | η-14 では未確認 (shader 側参照削除のみ完結) | B?-η-15+ で C++ 側 Skybox UBO 用途残存確認 → 用途 0 なら C++ 側削除候補 |
| 15 | cascade exposure 第10層 残 (SPIR-V 41 + normalMap 69 + depthMap 9 = 119 件) + 第11層 emergence 残 (non-opaque 6 件、Underwater/Terrain VERTEX 含む) = 計 125 件 η-15 移管 | sub-bundle 残量大 | B?-η-15 着手前 trace で系統別 Phase 分割継続推奨 (non-opaque 6 件 = 1 Phase 最優先、bare uniform → UBO wrap で η-8 §3.4 範式継承) |

---

**handoff doc 完。次 sub-bundle B?-η-15 着手は本 doc §10 推奨 scope を起点として、fresh context で実施。**
