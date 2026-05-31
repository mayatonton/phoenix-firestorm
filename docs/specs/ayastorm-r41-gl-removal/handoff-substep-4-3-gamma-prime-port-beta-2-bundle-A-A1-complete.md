# handoff: sub-step 4.3-γ'-port-β-2-bundle-A-A1 完遂

**作成日**: 2026-06-01
**親 commit**: `6f941c0a48` (sub-step 4.3-γ'-port-β-2-bundle-A-A1 完遂)
**HEAD**: `6f941c0a48` on `feature/ayastorm-r41-gl-removal`
**前段**: β-2-hook `fcf2b6c508` + β-2-bundle-A-prep `1434341904`
**次境界**: sub-step 4.3-γ'-port-β-2-bundle-A-A2 (fresh context 推奨)

---

## §1 A1 完遂 status

| 項目 | 値 |
|---|---|
| Scope | per-frame matrix uniform = FrameViewProj UBO binding (A) set=0/binding=0 |
| 注入 file 数 | **94 file** (prep doc estimate 67 → trace で 94 確定、AYA 「a」承認下で full scope) |
| 変更行数 | +1400 / -0 (94 file × 平均 ~15 line/file の 3-段 swap 注入) |
| 4 Agent 並列 patch 分担 | Agent1 24 + Agent2 26 + Agent3 22 + Agent4 22 = 94 全件 |
| AYA cold cache launch verify | **PASS** (hook fire 224 / parse fail 224 / SPIR-V 生成 0% controlled / `.shaderbin` 224 再生成 = GL regression 0 / 起動成立 + clean shutdown / crash 0) |
| AYA 「OK commit して」明示指示 | 2026-06-01 |

## §2 A1 で確定した範式 (A2-A7 継承必須)

### §2.1 FrameViewProj UBO canonical literal (補正後)

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=0, std140) uniform FrameViewProj {
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 projection_matrix;
    mat4 inv_proj;
    mat4 proj_mat;
    mat4 last_modelview_matrix;
    mat3 env_mat;
    mat3 normal_matrix;
    vec2 screen_res;
};
#endif
```

**補正履歴** (本 A1 session 内 AYA 「1 で、binding rules も同時補正」承認下):
- `mat4 env_mat` → `mat3 env_mat` (現状 GL declarator と一致、charter §3 #1 acceptance 担保)
- `vec4 screen_res` → `vec2 screen_res` (同上、post-r41 enrichment 案は bundle-A scope 外)
- binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (A) も同時補正済

### §2.2 3-段 swap pattern

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=N, binding=M, std140) uniform <BlockName> {
    <member declarations>
};
#else
uniform <type> <member1>;
uniform <type> <member2>;
...
#endif
```

- primary group (file の主要 uniform declaration block) で直接 swap
- isolated A1 line (主要 group 外で散在する member 宣言) は `#ifndef LL_VULKAN_GLSL ... #endif` で条件付き skip = body code 不変担保

### §2.3 hard rule 5 件 (Agent prompt 必須注入)

1. **UBO declaration byte-for-byte canonical**: §2.1 literal を全 file で完全一致コピー、type/member name/order 改変禁止
2. **3-段 swap pattern 厳守**: `#ifdef LL_VULKAN_GLSL ... #else ... #endif` の三段、`#if defined(LL_VULKAN_GLSL)` 等変形禁止
3. **binding 番号 §3 表遵守**: A2 = set=0/binding=1 (FrameLights) + set=0/binding=2 (FrameAtmosphere)、独自割当禁止
4. **1 file 1 patch**: primary group + isolated A1 line wraps を 1 commit 内、ファイル横断 share/共通化禁止
5. **skip list 13 file 機械的 exclusion**: `docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt` 照合必須

## §3 A2 着手境界 (次 sub-bundle 設計)

### §3.1 A2 scope

- **FrameLights** binding (B) set=0/binding=1 std140 = 8 member + array variant (binding rule artifact §2 (B) 参照、次 session で artifact 再読み込み必須)
- **FrameAtmosphere** binding (C) set=0/binding=2 std140 = 18 member (binding rule artifact §2 (C) 参照、同上)
- 推定 file 数 = **~30 file** (bundle-A-prep `1434341904` §3.1 estimate、A2-trace で実数確定 = A1 同様 estimate 楽観可能性あり、union of 8+18 member declarators)
- 推定 Agent 並列 = **2 Agent**

### §3.2 A2 着手前チェックリスト 9 件

1. `git fetch && git status` で HEAD = `6f941c0a48` 確認
2. AYAstorm 改変 11 file リスト確定 (skip list 13 から exemplar 2 除外):
   - Picker 2: `class1/deferred/fsObjectID{V,F}.glsl`
   - Cinematic BD 2: `cinematic_bd/class1/deferred/shadowUtil.glsl` + `cinematic_bd/class3/deferred/screenSpaceReflUtil.glsl`
   - Visual Realism 7: `class1/deferred/godrays{V,F}.glsl` + `class1/deferred/volumetricLightF.glsl` + `class3/deferred/volumetricLightF.glsl` + `class1/deferred/blurLight{V,F}.glsl` + `class1/windlight/atmosphericsFuncs.glsl`
3. skip list 確定 file = `docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt` (本 commit `6f941c0a48` HEAD で repo 内 source-of-truth)
4. binding rule artifact = `/tmp/bundle-A-binding-rules.md` (本 A1 session 内 §2 (A) 補正済、A2 で §2 (B) + (C) を再読み込み必須、artifact 自体は /tmp なので persist 性 fragile = 失効時は本 doc + binding rule の再生成)
5. bundle-A-prep doc 読了: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md` (commit `1434341904`)
6. β-2-hook complete doc 読了: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md` (commit `cecb9e6467`)
7. 本 A1 complete doc 読了 (この doc)
8. project memory 確認: `project_ayastorm_r41_vulkan_migration.md` (A1 完遂 + A2 着手境界 active 状態)
9. A2-trace の前に AYA 明示 「OK」承認待ち (feedback_one_step_at_a_time / feedback_no_auto_commit)

### §3.3 推奨 cadence 8 step (A1 範式継承)

1. **A2-trace**: Claude が 8+18 member 全件 grep + union 構築 + skip list 機械的除外 + 4 Agent prompt 分割 → AYA 確認 (file 数が prep estimate 30 と乖離する場合 AYA 判断)
2. **A2-prep**: Claude が 4 Agent prompt 構築 (binding rule artifact §2 (B)+(C) + skip list + 3-段 swap sample 注入) → AYA 確認
3. **A2-patch**: Agent 2 並列 (推定) 実行、AYA 明示 「OK」承認下で起動 (feedback_use_agents_proactively)
4. **A2-verify (self)**: Claude が patch 結果 self-verify (skip list 違反 0 + UBO canonical byte-for-byte + LL_VULKAN_GLSL 出現件数集計)
5. **A2-handoff to AYA**: Claude が AYA に build + cold cache launch verify 依頼 (必須コマンド = `autobuild configure / build` + cache 完全 clear `rm -rf ~/.ayastorm_x64/cache/` + launch)
6. **A2 measurement**: Claude が AYA launch 後 log 解析で hook fire 件数 + parse fail breakdown 取得 (期待 = parse fail 続行 0% SPIR-V 生成 controlled、A1+A2 で error breakdown の non-opaque uniforms 件数減少観測可能性あり = 確定値は次 session 内で)
7. **A2-commit**: Claude が verification log 除去 (本 A2 でも追加 LL_INFOS hook 不要設計、feedback_remove_verification_logs preventive 遵守) + AYA 明示 「OK commit して」承認下で 1 commit (β-2-hook + A1 範式継承、Co-Authored-By: Claude なし)
8. **A2-handoff doc 起草**: 本 A1 complete doc 範式継承で A2 complete doc 起草 (sub-bundle 単位 fresh context 推奨)

## §4 risks/caveats 9 件

1. **measurement plan §6.1 楽観性是正済**: A1 段階 measurement で確認、sub-bundle 単位 parse pass 率向上を期待せず、bundle-A 全体 (A1-A6) 完遂時の集合的閾値で評価 (handoff prep `1434341904` §6.1 「A1 単独 ~25%」estimate は実際 0% で正常)
2. **cold cache launch verify 必須**: prior `.shaderbin` GL binary cache hit で β-2-hook bypass 確認済 (本 A1 session 内で 1 回目 hook fire 1 件 vs 2 回目 cache clear 後 224 件で確定)、A2 verify 前 `rm -rf ~/.ayastorm_x64/cache/` 必須
3. **glslang missing #endif cascade artifact**: bundle-A 全体完遂で根本 error (binding/location) 解消時に自然消滅、bundle 進行で個別 fix 不要
4. **HAS_SKIN 内 primary group localized swap**: A1 で 4 file (pbralphaV/pbrglowV/pbropaqueV/shadowAlphaMaskV) 発見、A2 でも類似 edge case あり得る = trace 時に `#ifdef HAS_SKIN` 内側の primary group 検出必須
5. **isolated A1 line `#ifndef` wrap 設計**: A1 で 42 occurrence、A2 でも primary group 外の散在 member 宣言は `#ifndef LL_VULKAN_GLSL ... #endif` で wrap = body code 不変担保
6. **binding rule artifact persist 性**: `/tmp/bundle-A-binding-rules.md` は /tmp 配置で fresh OS reboot で失効、A2 session 開始時に artifact 存在確認 + 失効時は bundle-A-prep `1434341904` §2 表から再生成 (または本 doc §2.1 範式で member 列を逆推定)
7. **AYAstorm 改変 11 file untouched 維持**: A2 でも skip list 機械的 exclusion 遵守、Agent prompt に必須注入 (例: Cinematic BD 2 file には FrameLights/FrameAtmosphere 関連 uniform が存在しても本 commit scope 外)
8. **exemplar 2 (diffuseV/F) untouched 維持**: β-1 PoC 試作レール (sub-doc 03 §3.1.3) として固定、本 A2 でも untouched
9. **context budget concern**: A2 session 内で trace + prep + 2 Agent 並列 patch + self-verify + AYA handoff + measurement + commit + handoff doc の 8 step 全部、fresh context 推奨 (A1 session で残量 fragmenting 確認、sub-bundle 単位 session 分割 cadence の妥当性確認済)

## §5 AYA 承認境界 5 件

1. 本 A1-complete handoff doc commit (doc-only commit、AYA 確認下)
2. A2 着手指示 (明示 「OK」必要、feedback_no_auto_commit + feedback_one_step_at_a_time)
3. A2-patch Agent 並列起動 (Agent 2 並列推定、AYA 確認下)
4. A2 commit (AYA 「OK commit して」明示指示下のみ)
5. 11 file (Picker 2 + Cinematic BD 2 + Visual Realism 7) + exemplar 2 (diffuseV/F) untouched 境界

## §6 次 session 投入 prompt (fresh context 推奨)

以下を fresh context の次 Claude session に投入してください:

```
sub-step 4.3-γ'-port-β-2-bundle-A-A2 (per-frame light/atmosphere UBO 注入) 着手お願いします。

【前提】
- HEAD: 6f941c0a48 on feature/ayastorm-r41-gl-removal
- 前段: β-2-hook fcf2b6c508 → bundle-A-prep 1434341904 → bundle-A-A1 6f941c0a48 完遂

【読了必須】
1. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md (本 handoff doc、A1 範式 + A2 着手境界 + 推奨 cadence 8 step + risks 9 件)
2. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md (commit 1434341904、bundle-A 全体設計 §2 binding 表 + sub-bundle A1-A7 cadence)
3. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md (commit cecb9e6467、β-2-hook = per-program SPIR-V hook 設計)
4. docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt (13 file 機械的 exclusion source-of-truth)
5. memory: project_ayastorm_r41_vulkan_migration.md (A1 完遂 / A2 着手境界 active)
6. /tmp/bundle-A-binding-rules.md (存在確認 + §2 (B) FrameLights + (C) FrameAtmosphere 表を A2 注入 source-of-truth として再読み込み、/tmp 失効時は本 A1-complete doc §2.1 範式 + bundle-A-prep §2 表から再生成)

【A2 scope】
- FrameLights binding (B) set=0/binding=1 std140 = 8 member + array variant
- FrameAtmosphere binding (C) set=0/binding=2 std140 = 18 member
- 推定 file 数 ~30 file 2 Agent 並列 (A2-trace で実数確定、prep estimate 楽観可能性は A1 で実証済 = trace 結果が prep と乖離した場合 AYA 確認)

【cadence】(A1 範式継承 8 step、handoff doc §3.3)
1. A2-trace (Claude grep + union + skip list 機械的除外 + Agent prompt 分割 → AYA 確認)
2. A2-prep (4 Agent prompt 構築 → AYA 確認)
3. A2-patch (Agent 並列、AYA 明示 「OK」起動)
4. A2-verify (Claude self-verify)
5. A2-handoff (AYA build + cold cache launch verify 必須 = rm -rf ~/.ayastorm_x64/cache/ 含む)
6. A2 measurement (Claude log 解析)
7. A2-commit (AYA 「OK commit して」明示下)
8. A2 complete handoff doc 起草

【hard rule 5 件】(handoff doc §2.3)
1. UBO declaration byte-for-byte canonical
2. 3-段 swap pattern (#ifdef LL_VULKAN_GLSL ... #else ... #endif) 厳守
3. binding 番号 §3 表遵守 (B=set=0/binding=1、C=set=0/binding=2)
4. 1 file 1 patch、横断 share 禁止
5. skip list 13 file 機械的 exclusion (bundle-A-skip-list.txt 照合必須)

【feedback rule 10 件】(全 commit で遵守)
- feedback_doubt_self_first / feedback_no_scope_shrink / feedback_one_step_at_a_time / feedback_use_agents_proactively / feedback_self_verify_before_handoff / feedback_remove_verification_logs / feedback_no_claude_coauthor / feedback_no_auto_commit (AYA 「OK」明示下のみ commit) / feedback_proactive_handoff / feedback_explanation_lead_with_conclusion

【1st action】
A2-trace 着手前に AYA に明示「OK」確認。trace の前に上記 6 件読了済を 1 行 status で報告。
```

## §7 cross reference

- **handoff doc**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md` (commit `1434341904`) / `handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md` (commit `cecb9e6467`)
- **spec sub-doc**: 06 §1.2.2/§1.2.4/§3.1 sub-step 6.3 / 07 §3.1 sub-step 7.2-7.4
- **charter**: §3 #1 + §7.5
- **memory**: `project_ayastorm_r41_vulkan_migration.md` (γ'-port-β-2-bundle-A-A1 完遂 + γ'-port-β-2-bundle-A-A2 着手境界 active)
- **artifact**: `docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt` (repo 内) / `/tmp/bundle-A-binding-rules.md` (/tmp 配置、失効性あり)
- **feedback rules**: doubt_self_first / no_scope_shrink / one_step_at_a_time / use_agents_proactively / self_verify_before_handoff / remove_verification_logs / no_claude_coauthor / no_auto_commit / proactive_handoff / explanation_lead_with_conclusion

---

→ 次 action = AYA review + 本 handoff doc commit (doc-only、AYA 「OK」明示下) → fresh context で A2 着手 (上記 §6 prompt 投入)
