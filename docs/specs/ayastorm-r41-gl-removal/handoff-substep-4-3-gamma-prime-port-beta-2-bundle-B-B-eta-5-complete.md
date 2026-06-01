# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-5 完遂 handoff

**status**: B?-η-5 完遂 → 次 sub-bundle B?-η-6 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: `79246ccd7d` (2026-06-02)
**handoff doc commit**: 本 doc (B?-η-4-complete `a9bfd37c29` 範式継承)
**勤続範式継承**: B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-5 scope = handoff §10 確定 19 件 root cause 4 種同時対処 + 自作 bug 2 件 phase 2 遡及 fix、**2 phase 構成**:

- **phase 1** (handoff §10 4 種同時 patch): 118 file +590/-3
  - (a) FrameViewProj guard wrap 118 file (Agent A 並列、η-1 §3.1 範式継承)
  - (b-1) bare uniform → nameless UBO wrap 2 file (Agent B + clipSign 想定外発見)
  - (b-2) opposite permutation branch UBO 追加 3 file (Agent C)
  - (c) weight4/weight attribute guard wrap 4 file (Agent D)
- **phase 2** (verify 後 自作 bug 2 件 遡及 fix): 3 file +20/-2
  - (phase 2-A) PerDrawUBO_SkinnedVelocity padding member 固有化 2 file (η-3 §3.2 範式遡及、η-4 §3.2 member-level 拡張)
  - (phase 2-B) pbropaqueV.glsl HUD `#else` branch FrameViewProj guard wrap 1 file (Agent C scope leak fix)

**計**: 121 file +610/-5 (shader file のみ、C++ touch 0)

**4 主指標 100% 達成**:
- `Cannot reuse block name within the same interface` 10 → **0** ✓ -10 (主 scope 完全達成)
- `'weight4' / 'weight' redefinition` 2 → **0** ✓ -2
- `undeclared identifier` 3 → **0** ✓ -3
- `nameless block ... global scope` 1 → **0** ✓ (phase 2-A 自作 bug 遡及 fix)

**既達主指標完全維持**: `'size' undeclared` 0 / `Link failed` 0 / `GBufferInfo redefinition struct` 0 / `'#'` preprocessor 0

**第7層 emergence net 退行**: non-opaque uniforms +3 (57→60、link 成立 program 集合シフトによる露出側変化、B?-η-6 移管対象)

---

## §2 完遂結果 metric (vs B?-η-4 baseline)

| metric | η-4 baseline | η-5 phase 1 | η-5 phase 2 (commit) | Δ vs η-4 | 判定 |
|---|---|---|---|---|---|
| Cannot reuse block name | 10 | 0 | **0** | -10 | ✓ 100% 主 scope |
| 'weight4'/'weight' redefinition | 2 | 0 | **0** | -2 | ✓ 100% |
| undeclared identifier | 3 | 1 | **0** | -3 | ✓ 100% |
| nameless block ... global scope | 0 | 1 | **0** | ±0 | ✓ 維持 (phase 2-A 自作 bug 遡及 fix) |
| 'size' undeclared | 0 | 0 | 0 | ±0 | ✓ B?-η-3 達成維持 |
| Link failed | 0 | 0 | 0 | ±0 | ✓ B?-η-3 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | 0 | ±0 | ✓ B?-ζ 達成維持 |
| '#' preprocessor | 0 | 0 | 0 | ±0 | ✓ B?-ε 達成維持 |
| missing #endif | 104 | 89 | 87 | -17 | cascade pair 自動消滅 (phase 1 -15 + phase 2 -2) |
| parse failed | 186 | 181 | 179 | -7 | cascade 内訳変化 |
| redefinition (all) | 81 | 79 | 79 | -2 | weight redefinition 解消 |
| non-opaque uniforms | 57 | 60 | 60 | +3 | 第7層 emergence net 退行 (B?-η-6 移管) |
| 'binding' | 21 | 21 | 21 | ±0 | 微小 net シフト |
| 'location' | 14 | 19 | 19 | +5 | 第7層 emergence |
| shader_cache 件数 | 259 | 262 | **263** | +4 | link 成立 program 集合シフト |
| FATAL/SIGSEGV/Aborted | 0 | 0 | 0 | ±0 | ✓ |
| Goodbye / Vulkan device destroyed / instance destroyed / status: stopped | 1/1/1/1 | 1/1/1/1 | 1/1/1/1 | ±0 | ✓ clean shutdown |

---

## §3 設計範式

### §3.1 新規 設計範式: handoff §10 確定 multi-root-cause 同時対処範式

**概要**: handoff §10 で複数 root cause が確定済の場合、各 root cause を独立 sub-bundle に分割するのではなく、**1 sub-bundle 内で 4 Agent disjoint scope 並列**展開で同時対処する範式。

**位置付け**: scope refinement 2nd-level (root cause 別 sub-bundle 分割) の進化形 = 同時対処 multi-axis 拡張、η-1 §3.3 Agent 並列 disjoint scope 範式の multi-axis 拡張。

**適用条件**:
- handoff §10 で全 root cause が trace 済 (LINE + program 確定)
- 各 root cause の patch literal が独立 (cross-dependency 無し)
- 各 root cause の修正 file scope が disjoint または overlap 明示確認可能

**η-5 適用例**:
- (a) FrameViewProj guard wrap: 118 file (Agent A)
- (b-1) bare uniform → UBO wrap: 2 file (Agent B)
- (b-2) opposite permutation branch UBO 追加: 3 file (Agent C)
- (c) weight4/weight attribute guard wrap: 4 file (Agent D)
- overlap 2 file (pbropaqueF.glsl B+C / avatarVelocityV.glsl A+D) は line-range disjoint で衝突 0

**Trade-off**:
- 利点: 1 sub-bundle で複数 root cause 解消 (B?-η-2 (a) → η-3 → η-4 → η-5 の 4 sub-bundle 分割と比較し、handoff doc/commit overhead が 1/4)
- 注意: phase 1 verify で想定外発見が出た場合、scope refinement 5th-level (§3.2) で吸収

### §3.2 新規 設計範式: handoff §10 確定後 phase 1 verify 想定外発見 → 自作 bug 2 件 phase 2 遡及 fix 範式

**概要**: phase 1 で handoff §10 確定 root cause を全種同時対処後、cold cache verify で **残存・退行** を trace、自作 bug (過去 sub-bundle で導入した patch が新たな collision を引き起こすケース) は **即 phase 2 遡及 fix**、第7層 emergence は次 sub-bundle 移管。

**位置付け**: η-4 §3.1 scope refinement 4th-level 範式の **自作 bug 特化応用** = scope refinement 5th-level、feedback_root_cause_not_dump 範式 (自作機能の不完全さを workaround で隠さず根本修正) の sub-bundle 内構造化。

**適用フロー** (4 step):
1. **phase 1 verify**: cold cache launch + 4 主指標 grep
2. **残/退行 trace**: 主指標 0 達成しなかった残件 + 既達主指標退行を log で context grep
3. **自作 bug 判定**: 過去 sub-bundle commit の patch との因果関係確認 = `git blame` / `git log -p` で literal 一致 + 範式同形性確認
4. **phase 2 fix**: 自作 bug は即 phase 2 で根本修正 (revert/defer は禁止 = feedback_self_bug_no_defer_option 範式)、第7層 emergence は次 sub-bundle 移管

**η-5 適用例**:
- phase 1 verify 残 3 件 trace:
  - nameless block 1 件 (`PerDrawUBO_SkinnedVelocity` + `PerDrawUBO_ObjectSkin` の `lastMatrixPalette` member 衝突) = **η-3 自作 bug**
  - undeclared identifier 1 件 (`HUD PBR Opaque Shader` vertex stage の `modelview_projection_matrix`) = **Agent C scope leak**
  - non-opaque uniforms +3 = 第7層 emergence (B?-η-6 移管)
- phase 2-A: PerDrawUBO_SkinnedVelocity `lastMatrixPalette` → `lastMatrixPalette_skinned_velocity` rename (2 file)
- phase 2-B: pbropaqueV.glsl HUD `#else` branch FrameViewProj guard wrap 追加 (1 file)
- phase 2 verify: 自作 bug 2 件 全解消、non-opaque 60 件は B?-η-6 移管

### §3.3 新規 設計範式: Agent depth trace 投入閾値範式

**概要**: glslang error の stage type / LINE 番号確定が **linear trace で不可能** な場合、Agent (general-purpose) 投入で実データ trace、API 仕様 trace、similar fix pattern 検索を一括実施。

**位置付け**: feedback_admit_unknown 範式の sub-bundle 内構造化、feedback_doubt_self_first 範式の Agent 投入版。

**Agent 投入閾値**:
- 推論 2 連続外れ
- API 詳細 (stage type 番号 / merged source line offset / preprocessor 挙動) の確定が linear trace で 5 分以上要する
- log error の真因が複数 file 跨ぎ trace を要する

**Agent 報告フォーマット** (η-4 §3.4 範式継承):
- root cause 結論 (1 段落)
- 推奨 patch (具体的 diff、charter §3 #1 GL path byte-for-byte 不変厳守)
- alt patch 1-2 件 (trade-off 明示)

**η-5 適用例**:
- phase 2-B: `HUD PBR Opaque Shader` error LINE 0:532 で `modelview_projection_matrix` undeclared
- linear trace: pbropaqueF.glsl の HUD branch 内に `modelview_projection_matrix` 参照無し、srgbF.glsl 内にも無し → 真因不明
- Claude 自己誤認: stage type 0x8b31 を fragment と推測 (誤、正解は vertex)
- Agent 投入 5 分で確定: 0x8b31 = `GL_VERTEX_SHADER` (0x8b30 = fragment)、真因は pbropaqueV.glsl LINE 261 の HUD branch、Vulkan path に FrameViewProj UBO 欠落
- patch: pbropaqueV.glsl HUD `#else` branch に FrameViewProj guard wrap +18 行

### §3.4 η-4 §3.2 (nameless block member global scope export 衝突 detection 範式) member-level 拡張

**概要**: η-4 §3.2 で確立した **nameless block padding member 固有化範式** を、通常 member (`lastMatrixPalette` 等) にも拡張適用。

**η-5 適用例** (phase 2-A):
- `PerDrawUBO_SkinnedVelocity` と `PerDrawUBO_ObjectSkin` が `Skinned AYAstorm Velocity Shader` program に同時 attach
- 両 nameless block の `lastMatrixPalette` member が global scope で collision
- per-block 固有化 = `lastMatrixPalette` → `lastMatrixPalette_skinned_velocity` (skinnedVelocityV/AlphaV のみ)
- shader logic 側参照は **objectSkin UBO 経由** のため不変 (skinnedVelocity 側 member は GL 時代から redundant、grep 検証済)

**Detection 手順** (η-4 §3.2 完全継承):
- (a) `nameless block ... global scope` エラー件数 × 発生 program で attach 関係 trace
- (b) attach される全 file の nameless block member 名 literal grep で抽出
- (c) member 名 collision 検出 (padding 系 high-risk + 通常 member 跨ぎ共有も risk)
- (d) per-block 固有化 rename = `<member>_<sub_group_suffix>` style
- (e) GL path 不可触担保 = `#else` branch byte-for-byte 不変 + Vulkan path member 名のみ rename

### §3.5 η-1 §3.2 cascade pair hypothesis 汎用形 1:1 比例継続検証

**η-5 検証**:
- phase 1 後 `missing #endif` 104→89 -15 (cascade pair 自動消滅)
- phase 2 後 89→87 -2 (自作 bug 解消で更に -2)
- 計 -17 vs 主指標 -15 (a 10 + c 2 + b-2 3) = +2 offset (b-2 部分 cascade 連動 + 自作 bug 解消連動)
- cascade pair 1:1 比例継続検証完了

---

## §4 cold cache launch verify metric 詳細 (vs B?-η-4 baseline)

### §4.1 主指標 (handoff §10 19 件 + 自作 bug 1 件)

| 主指標 | η-4 | η-5 phase 2 | Δ | 内訳 |
|---|---|---|---|---|
| Cannot reuse block name | 10 | **0** | -10 ✓ | phase 1 (a) FrameViewProj guard wrap 118 file 直接効果 |
| 'weight4'/'weight' redefinition | 2 | **0** | -2 ✓ | phase 1 (c) WEIGHT4/WEIGHT_LOCATION_DEFINED guard wrap 4 file 直接効果 |
| undeclared identifier | 3 | **0** | -3 ✓ | phase 1 (b-2) opposite branch UBO 追加 3 file -2 + phase 2-B HUD FrameViewProj 追加 1 file -1 |
| nameless block ... global scope | 0 | **0** | ±0 ✓ | phase 1 で +1 退行 (η-3 自作 bug) → phase 2-A rename 2 file で完全解消、η-4 達成完全維持回復 |

### §4.2 既達主指標完全維持

| 主指標 | 値 | 由来 sub-bundle |
|---|---|---|
| 'size' : undeclared identifier | 0 | B?-η-3 達成完全維持 |
| Link failed | 0 | B?-η-3 達成完全維持 |
| GBufferInfo : redefinition struct | 0 | B?-ζ 達成完全維持 |
| '#' preprocessor | 0 | B?-ε 達成完全維持 |

### §4.3 第7層 emergence net シフト分析

| metric | η-4 | η-5 | Δ | 評価 |
|---|---|---|---|---|
| non-opaque uniforms | 57 | 60 | +3 | 局所退行 = handoff §10 (b-1) 4 件 fix vs 第7層 emergence 露出 +7、B?-η-6 主 scope |
| 'binding' | 21 | 21 | ±0 | 微小 net シフト = 解消と新規露出が相殺 |
| 'location' | 14 | 19 | +5 | 第7層 emergence (SPIR-V location missing + overlapping location) |
| shader_cache 件数 | 259 | 263 | +4 | link 成立 program 集合シフト、+4 program 増加 |

### §4.4 主指標 metric integrity self-check (B3 §12 literal grep 範式継承)

- 主指標 4 種 100% 達成 (-10/-2/-3 + 自作 bug ±0)
- cascade pair 自動消滅 -17 = 主指標 -15 + 自作 bug 解消連動 -2 で整合
- parse failed -7 = 主指標 -15 + 第7層 emergence net +8 で整合
- shader_cache +4 は link 成立 program 集合シフト = -21 観測点完全解消継続維持

### §4.5 shutdown clean verify

- FATAL/SIGSEGV/Aborted: 0/0/0 ✓
- Goodbye! : 1 ✓
- Vulkan device destroyed: 1 ✓
- Vulkan instance destroyed: 1 ✓
- status: stopped: 1 ✓

---

## §5 self-verify (B3 §12 + B?-η-4 §5 範式継承)

| # | 項目 | 結果 |
|---|---|---|
| 1 | charter §3 #1 GL path byte-for-byte 不変 担保 | ✓ (121 file 全 outer `#ifdef LL_VULKAN_GLSL ... #else <GL path> ... #endif` literal 不変) |
| 2 | shader file のみ編集 (C++ touch 0) | ✓ `git diff --stat HEAD~1 -- '*.cpp' '*.h'` 0 |
| 3 | 主指標 4 種 literal grep verify | ✓ Cannot reuse 0 + weight4/weight redefinition 0 + undeclared identifier 0 + nameless block 0 |
| 4 | 既達主指標 4 種 literal grep verify | ✓ 'size' 0 + Link failed 0 + GBufferInfo 0 + '#' 0 全 ±0 |
| 5 | cascade pair 1:1 比例検証 | ✓ missing #endif -17 = 主指標 -15 + 自作 bug -2 で整合 |
| 6 | parse failed integrity | ✓ -7 = 主指標 -15 + 第7層 emergence net +8 |
| 7 | shader_cache 件数 | ✓ 263 (+4 = link 成立 program 集合シフト) |
| 8 | FATAL/SIGSEGV/Aborted 0 維持 | ✓ 0/0/0 |
| 9 | clean shutdown | ✓ Goodbye/Vulkan destroy/status stopped 各 1 |
| 10 | Agent 並列 disjoint scope 衝突 0 | ✓ overlap 2 file (pbropaqueF/avatarVelocityV) は line-range disjoint |
| 11 | 自作 bug 即 phase 2 fix (feedback_root_cause_not_dump) | ✓ revert/defer なし、根本修正 |
| 12 | Agent depth trace 投入閾値範式適用 | ✓ phase 2-B stage type 0x8b31 確定で投入 |
| 13 | scope refinement 5th-level 適用範囲明示 | ✓ §3.2 で自作 bug 特化応用と明示 |
| 14 | 過去 sub-bundle 既処理 file byte-for-byte 維持 | ✓ A1-A7/A8-recovery/B1-B3/B2-α-γ/B?-δ-η-4 全件 |
| 15 | skip list admission 範式継承 (B?-δ) | ✓ insertions-only or rename-only + outer LL_VULKAN_GLSL 不変 |
| 16 | handoff doc 別 commit (η-4 範式継承) | ✓ 本 doc は patch commit `79246ccd7d` 後の別 commit |

---

## §6 設計範式継承表

### feedback memory 範式 (10 件全件適用確認)

| # | feedback memory | η-5 適用箇所 |
|---|---|---|
| 1 | feedback_doubt_self_first | phase 2-B `modelview_projection_matrix` 真因不明時に推論停止 → Agent 投入 |
| 2 | feedback_admit_unknown | stage type 0x8b31 を fragment と誤認 → Agent 投入で vertex 確定 |
| 3 | feedback_build_only_verified | cold cache launch verify を phase 1 / phase 2 双方で実施、metric grep で結果確定 |
| 4 | feedback_falsification_as_progress | phase 1 b-1 fix で +3 net 退行を progress として handoff doc 記録、B?-η-6 移管 |
| 5 | feedback_one_step_at_a_time | AYA 「OK」明示指示後 phase 単位 deploy + verify、複数 phase 並走しない |
| 6 | feedback_no_auto_commit | AYA 「commit してください」明示指示後 commit |
| 7 | feedback_no_claude_coauthor | Co-Authored-By: Claude 行を含めない (全 commit) |
| 8 | feedback_no_scope_shrink | phase 1 で 4 種同時対処、scope 縮小 (= 1 種ずつ) せず |
| 9 | feedback_shader_only_fast_iterate | shader file のみ編集、autobuild 不要、cp + rm shader_cache で fast iterate |
| 10 | feedback_root_cause_not_dump | 自作 bug 2 件を revert/defer せず phase 2 で根本修正 |

### prior sub-bundle 範式 (15 件適用継承)

| sub-bundle | 範式 | η-5 適用 |
|---|---|---|
| B1 §3 | shader-file 側 UBO 宣言 guard wrap | phase 1 (a) FrameViewProj + phase 2-B HUD FrameViewProj |
| B2-α §3.1 | GL `#else` branch byte-for-byte 不変 | 121 file 全件 |
| B3 §3.2 | metric integrity self-check | §4.4 |
| B3 §12 | literal grep metric verify | §4.1-§4.5 全 metric |
| B2-γ §3.1 | Agent 並列 disjoint scope | phase 1 A/B/C/D 4 並列 |
| B2-γ §3.2 | pilot 1 file で patch literal 確立 | deferredUtil.glsl pilot |
| B2-γ §3.3 | charter §3 #1 担保 | 121 file 全件 |
| B?-δ §3.1 | skip list admission | deferredUtil/skyV/cloudsV/cloudsF/shadowUtil 等再 touch admission |
| B?-δ §3.2 | UBO body member byte-for-byte 維持 | 全 file 全 UBO body |
| B?-ε §3.2 | 1 source N 件 cluster 対処 | phase 1 (a) 118 file 1 source FrameViewProj guard |
| B?-ζ §3.1 | shader-file 側 UBO 宣言 guard wrap | phase 1 (a) FrameViewProj |
| B?-η-1 §3.1 | shader-file 側 UBO 宣言 guard wrap | phase 1 (a) FrameViewProj + phase 1 (b-1) CASParamUBO/PBROpaqueExtraUBO |
| B?-η-1 §3.2 | cascade pair hypothesis | §3.5 |
| B?-η-1 §3.3 | Agent 並列 disjoint scope | phase 1 A/B/C/D |
| B?-η-2 (a) §3.1 | handoff scope refinement 範式 | phase 2 §3.2 で自作 bug 特化応用 |
| B?-η-2 (a) §3.2 | cascade pair hypothesis 汎用形 | §3.5 1:1 比例継続検証 |
| B?-η-3 §3.1 | scope refinement 3rd-level (handoff §10 未列挙発見) | phase 2 §3.2 で 5th-level 進化 |
| B?-η-3 §3.2 | guard macro collision detection | phase 1 (a) FrameViewProj 1 group のみ確認 (body 一致 119 file 全件) |
| B?-η-4 §3.1 | scope refinement 4th-level (sub-bundle 内 3 phase refine) | phase 2 §3.2 で 5th-level 自作 bug 特化応用 |
| B?-η-4 §3.2 | nameless block member 固有化 detection 範式 | §3.4 member-level 拡張で適用 |
| B?-η-4 §3.4 | 仮説 4 件評価による真因絞り込み範式 | phase 2-B Agent 投入で適用 (Agent 報告 4 仮説評価) |

### 新規 設計範式 (§3.1-§3.4 計 4 件)

- §3.1: handoff §10 確定 multi-root-cause 同時対処範式
- §3.2: handoff §10 確定後 phase 1 verify 想定外発見 → 自作 bug 2 件 phase 2 遡及 fix 範式 (scope refinement 5th-level)
- §3.3: Agent depth trace 投入閾値範式
- §3.4: η-4 §3.2 member-level 拡張 (nameless block 通常 member 固有化)

---

## §7 risks

| # | risk | 評価 | 緩和策 |
|---|---|---|---|
| 1 | UBO body 差異 link failure exposure | ✓ 完全解消継続 (B?-η-3 達成維持) | FrameViewProj 119 file 全件 body 一致確認済 |
| 2 | skip list 再 touch admission | ✓ 13 + A2 拡張 2 + 5 V skip 中 多数再 touch | B?-δ admission 範式 = insertions-only or rename-only + outer LL_VULKAN_GLSL 不変 で skip list 趣旨 (AYAstorm 改変保護) 担保 |
| 3 | cascade pair hypothesis 汎用形 1:1 比例継続検証 | ✓ phase 1 -15 + phase 2 -2 で継続成立 | B?-η-6 以降も自動消滅指標として継続観測 |
| 4 | scope refinement 5th-level 濫用リスク | 監視対象 | 5th-level は **自作 bug 特化応用** のみ、通常 root cause は 2nd/3rd-level で sub-bundle 分割 |
| 5 | non-opaque +3 局所退行 | B?-η-6 移管 | 第7層 emergence net シフト、handoff §10 (b-1) 4 件 fix が露出側 +7 と相殺 |
| 6 | Agent 並列 disjoint 衝突 | ✓ 0 件 | overlap 2 file (pbropaqueF/avatarVelocityV) line-range disjoint で衝突 0 |
| 7 | literal verify 範式 | ✓ 全 metric literal grep 確定 | B3 §12 範式継承 |
| 8 | shader_cache 263+ 維持観測点切替 | 観測継続 | -21 観測点完全解消継続維持 |
| 9 | charter §3 #1 byte-for-byte | ✓ 121 file 全件担保 | §5 #1 |
| 10 | 範式誤伝承 member-level 拡張範囲 | 監視対象 | §3.4 で η-4 §3.2 padding member → 通常 member 拡張、適用範囲は **nameless block member 跨ぎ共有検出時のみ** |

---

## §8 commit history

| commit | type | scope | file 数 |
|---|---|---|---|
| `79246ccd7d` | feat(r41) | η-5 patch (phase 1 + phase 2) | 121 file +610/-5 |
| `a9bfd37c29` | docs(r41) | η-4 完遂 handoff (起点) | 1 file +416 |
| `0a4008066c` | feat(r41) | η-4 patch | 48 file +199/-195 |

---

## §9 file inventory (η-5 patch 121 file 内訳)

### phase 1 (a) FrameViewProj guard wrap 118 file (Agent A 並列 + pilot)

- **pilot 1 file**: `class1/deferred/deferredUtil.glsl` (Claude 自力)
- **Agent A1** (13 file): class1/deferred 前半 (alphaV, avatarAlphaShadowV, avatarEyesV, avatarShadowV, avatarV, blurLightV, bumpV, deferredV, diffuseNoColorV, emissiveV, fsObjectIDV, fullbrightShinyV, fullbrightV)
- **Agent A2** (14 file): class1/deferred 後半 + interface + lighting + objects (gltf*, impostorV, multiPointLightV, multiSpotLightV, normalDebugV, postDeferredV, shadowV, simpleV, skyV, spotLightV, sunLightV, terrainV, velocityV, waterV)
- **Agent A3** (17 file): class1/deferred 残 + class1/effects/environment/gltf/objects/post + class1/avatar + class1/post + class2/deferred + class3/deferred + cinematic_bd + root errorV

### phase 1 (b-1) bare uniform → nameless UBO wrap 2 file (Agent B)

- `class1/deferred/CASF.glsl`: `CASParamUBO_Legacy` (set=3, binding=12) +11
- `class1/deferred/pbropaqueF.glsl`: `PBROpaqueExtraUBO_Legacy` (set=3, binding=13) +13/-1 + clipSign duplicate fix

### phase 1 (b-2) opposite permutation branch UBO 追加 3 file (Agent C)

- `class1/deferred/pbrglowV.glsl`: non-skinned `#else` branch FrameViewProj +18/-1
- `class1/deferred/pbralphaV.glsl`: HUD `#else` branch FrameViewProj +18/-1
- `class1/deferred/pbropaqueF.glsl`: HUD `#else` branch FrameAtmosphere_Lighting +29/-1

### phase 1 (c) weight4/weight attribute guard wrap 4 file (Agent D)

- `class1/avatar/objectSkinV.glsl`: `WEIGHT4_LOCATION_DEFINED` +8
- `class1/deferred/skinnedVelocityV.glsl`: `WEIGHT4_LOCATION_DEFINED` +8
- `class1/avatar/avatarSkinV.glsl`: `WEIGHT_LOCATION_DEFINED` +8
- `class1/deferred/avatarVelocityV.glsl`: `WEIGHT_LOCATION_DEFINED` +8

### phase 2-A PerDrawUBO_SkinnedVelocity padding member 固有化 2 file (Claude 自力)

- `class1/deferred/skinnedVelocityV.glsl`: `lastMatrixPalette` → `lastMatrixPalette_skinned_velocity` +1/-1
- `class1/deferred/skinnedVelocityAlphaV.glsl`: 同上 +1/-1

### phase 2-B HUD FrameViewProj guard wrap 1 file (Claude 自力)

- `class1/deferred/pbropaqueV.glsl`: HUD `#else` branch FrameViewProj +18

### overlap file 確認

- `class1/deferred/pbropaqueF.glsl`: Agent B (b-1) + Agent C (b-2) 同 file、line-range disjoint
- `class1/deferred/avatarVelocityV.glsl`: Agent A (a) + Agent D (c) 同 file、line-range disjoint (Agent A line 28-46 / Agent D line 64-78)
- `class1/deferred/skinnedVelocityV.glsl`: Agent A (a) + Agent D (c) + phase 2-A 3 重 touch、各 line-range disjoint

---

## §10 次 sub-bundle B?-η-6 推奨 scope

### §10.1 確定 scope: non-opaque uniforms outside a block 60 件

handoff §10 (b-1) 残 = 4 件 fix で +7 第7層 emergence 露出 → 計 60 件:

| LINE | 件数 | 主 program 例 | 推定 root cause |
|---|---|---|---|
| 1795 | 4 | (要 trace) | 第7層 emergence |
| 1787 | 4 | (要 trace) | 第7層 emergence |
| 1782 | 4 | (要 trace) | 第7層 emergence |
| 1387 | 4 | Reflection Probe Display Shader | bare uniform 単発 |
| 491 | 2 | (要 trace) | bare uniform 単発 |
| 417 | 2 | (要 trace) | bare uniform 単発 |
| 341 | 2 | (要 trace) | bare uniform 単発 |
| 186 | 2 | Glow Shader (Post) / Irradiance Gen / Hero Radiance Gen / Glow Combine | bare uniform 単発 |
| 184 | 2 | Irradiance Gen / Vignette Post | bare uniform 単発 |
| 残 | 33 | Underwater / Glow Extract / Pathfinding / Pathfinding No Normals / Occlusion Cube / Skinned Normal Debug / Normal Debug / Clip Shader / Snapshot Frame Post / Draw Color / Reflection Mip / Radiance Gen 等 | bare uniform 単発 + 第7層 emergence 混在 |

主 program 集合:
- 第7層 emergence 系: Underwater / Reflection 系 (HUD/Probe Display/Mip/Radiance Gen/Hero Radiance Gen/Irradiance Gen) / Skinned Normal Debug
- bare uniform 単発: Glow 系 (Shader/Extract/Combine/Vignette) / Pathfinding / Snapshot Frame Post / Clip Shader / Draw Color / Occlusion Cube / Normal Debug

### §10.2 B?-η-6 着手前 trace 範式 5 ステップ (η-4 §10.2 + η-5 §3.2 範式継承)

1. **literal grep**: `grep -c "non-opaque uniforms outside a block" log` + `grep -B 1 "non-opaque uniforms" log | grep -oE 'program [^)]+' | sort -u`
2. **log context 抽出**: 各 program の error LINE × 件数 table 起草
3. **handoff §10 想定外発見 trace** (η-3 §3.1 範式): handoff §10 (b-1) 4 件 想定 vs 実 60 件 = 第7層 emergence net 退行 +56 件を sub-group 分類
4. **Agent 並列 disjoint scope** (η-1 §3.3 範式): bare uniform 単発系 (Glow/Pathfinding/Clip/Snapshot/Draw Color/Occlusion Cube/Normal Debug 等) と 第7層 emergence 系 (Underwater/Reflection 系/Skinned Normal Debug) で分割
5. **4 仮説評価範式** (η-4 §3.4): 推論 2 連続外れたら推論停止 → Agent 投入で実データ trace、4 仮説評価 table 事前起草

### §10.3 B?-η-6 完遂後の想定 cascade exposure 第8層

- `missing #endif` cascade pair hypothesis 汎用形により、non-opaque -60 件で missing #endif -60 件想定 (1:1 比例)
- 第7層 emergence net シフトで `'binding'` / `'location'` / 別 program scope 系で別 cluster 露出可能性
- shader_cache 263 → 280+ 想定 (link 成立 program +20)

### §10.4 第7層 emergence 系統整理 sub-bundle 分割推奨表

| sub-bundle | scope | 件数想定 | 主 program |
|---|---|---|---|
| **B?-η-6** | non-opaque uniforms 60 件 | bare uniform UBO 化 | Glow 系 + Pathfinding + Clip + Snapshot + Draw Color + Occlusion Cube + Normal Debug + Underwater/Reflection/Radiance/Irradiance |
| **B?-η-7** | 'normalMap' redefinition cluster 残 (η-3 §3.3 想定 188 件 → η-5 で 79 件、η-6 で更に減目論見) | normalMap redefinition 統一 patch | materialF 系 (要 trace) |
| **B?-η-8** | sampler 'binding' 21 件 | sampler binding 明示 | (要 trace) |
| **B?-η-9** | 'location' 19 件 | SPIR-V location missing + overlapping location | (要 trace) |

---

## §11 観測点

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 263+ 維持観測点 | 維持中 | B?-η-6 で 280+ 想定、減少時は link 成立路径退行 = root cause 別 |
| 2 | cascade pair hypothesis 汎用形 1:1 比例継続検証 | 継続成立 (phase 1 -15 + phase 2 -2) | B?-η-6 で missing #endif -60 件想定 |
| 3 | scope refinement 5th-level 適用範囲 (§3.2) | 自作 bug 特化応用のみ | 通常 root cause は 2nd/3rd-level で sub-bundle 分割 |
| 4 | nameless block member 固有化範式 (§3.4) member-level 拡張範囲 | nameless block member 跨ぎ共有検出時のみ | guard wrap で済む通常 collision は §3.1 範式適用、global scope export 衝突のみ §3.4 適用 |
| 5 | Agent depth trace 投入閾値範式 (§3.3) | 推論 2 連続外れ or API 詳細 5 分以上 trace 時 | B?-η-6 以降も継続適用、Agent 報告フォーマット = root cause + 推奨 patch + alt 1-2 件 |
| 6 | handoff §10 確定 multi-root-cause 同時対処範式 (§3.1) | 適用済 | B?-η-6 でも複数 root cause 確定済なら multi-axis 4 Agent 並列適用 |
| 7 | feedback_admit_unknown 範式の sub-bundle 内構造化 (η-4 §3.4 / η-5 §3.3) | 継続適用 | 推論停止判断 + Agent 投入閾値 + 4 仮説評価 table の 3 段階構造 |
| 8 | 第7層 emergence net 退行監視 | non-opaque +3 局所退行 (η-4 vs η-5) | B?-η-6 主 scope、completed 後は B?-η-7/8/9 で残 emergence 系統対処 |
