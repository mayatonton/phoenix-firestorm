# handoff: sub-step 4.3-γ'-port-β-2-bundle-A-A2 完遂

**作成日**: 2026-06-01
**親 commit**: `9f77f875db` (sub-step 4.3-γ'-port-β-2-bundle-A-A2 完遂)
**HEAD**: `9f77f875db` on `feature/ayastorm-r41-gl-removal`
**前段**: β-2-hook `fcf2b6c508` + bundle-A-prep `1434341904` + bundle-A-A1 `6f941c0a48`
**次境界**: sub-step 4.3-γ'-port-β-2-bundle-A-A3 (fresh context 推奨)

---

## §1 A2 完遂 status

| 項目 | 値 |
|---|---|
| Scope | per-frame light/atmosphere uniform = FrameLights binding (B) set=0/binding=1 + FrameAtmosphere binding (C) set=0/binding=2 |
| 注入 file 数 | **52 file** (prep doc estimate 30 → trace で 54 → previewV/multiPointLightF skip list 拡張で 52 確定、AYA「推奨方針で実行」承認下で canonical 補正方針採用) |
| 変更行数 | +1315 / -5 (5 deletion = cloudsV/skyV の non-canonical uniform を swap 外へ move した re-organization、GL 機能変化 0) |
| 2 Agent 並列 patch 分担 | Agent1 26 (class1/deferred) + Agent2 26 (rest) = 52 全件 |
| 注入 metric | FrameLights block = 21 file (Agent1 6 + Agent2 15) / FrameAtmosphere block = 40 file (Agent1 24 + Agent2 16) / 両 block 同居 = 10 file (Agent1 4 + Agent2 6) |
| isolated `#ifndef LL_VULKAN_GLSL` wrap | 14 occurrence (Agent1 3 + Agent2 11) |
| AYA cold cache launch verify | **PASS** (hook fire 224 / parse fail 224 / SPIR-V 生成 0% controlled / `.shaderbin` 223 再生成 = GL regression 0 / 起動成立 + clean shutdown / crash 0 / GL shader compile/link fail 0) |
| AYA 「OK commit して」明示指示 | 2026-06-01 |

## §2 A2 で確定した範式 (A3-A7 継承必須)

### §2.1 FrameLights UBO canonical literal (補正後、A1 範式継承)

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=1, std140) uniform FrameLights {
    int  sun_up_factor;
    vec3 sun_dir;
    vec3 moon_dir;
    vec4 waterPlane;
    vec4 light_position[8];
    vec3 light_direction[8];
    vec4 light_attenuation[8];
    vec3 light_diffuse[8];
    vec2 light_deferred_attenuation[8];
};
#endif
```

**補正履歴** (本 A2 session 内、AYA「可能な限り近づけられる方針を推奨案として実行」承認下):
- `float sun_up_factor` → `int sun_up_factor` (actual GLSL 全 file int で declared)
- scalar `vec4 light_position` → `vec4 light_position[8]` (HW lights array、`LLPipeline::setupHWLights` 8 slot 固定)
- scalar `vec3 light_diffuse` → `vec3 light_diffuse[8]` (同上、canonical は vec4 だったが actual vec3)
- scalar `vec4 light_attenuation` → `vec4 light_attenuation[8]` (同上)
- scalar `vec3 light_direction` → `vec3 light_direction[8]` (同上)
- `vec2 light_deferred_attenuation[8]` を (B-array) Promoted form から canonical 編入 (pbralphaF/pbrmetallicroughnessF のみ参照、他 50 file は declared-but-unused 合法)
- B-array canonical `light_count` + `light_attenuation_local` は actual 0 件で除外
- binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (B) は本 commit で **更新していない** (canonical correction は本 handoff doc を A3+ source-of-truth として優先)

### §2.2 FrameAtmosphere UBO canonical literal (binding rule §2 (C) 完全継承、補正なし)

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=2, std140) uniform FrameAtmosphere {
    vec3  sunlight_color;
    float scene_light_strength;
    vec3  moonlight_color;
    float haze_density;
    vec3  ambient_color;
    float density_multiplier;
    vec3  blue_horizon;
    float distance_multiplier;
    vec3  blue_density;
    float max_y;
    vec3  glow;
    float sky_sunlight_scale;
    float sky_ambient_scale;
    float sky_hdr_scale;
    int   classic_mode;
    int   cube_snapshot;
    float minimum_alpha;
    float max_cof;
    float _pad_atm0;
    float _pad_atm1;
};
#endif
```

**補正なし** = actual GLSL 全 18 member type 一致確認済 (A2 type survey で全 file grep)

### §2.3 3-段 swap pattern (A1 範式継承 + conditional injection 拡張)

```glsl
#ifdef LL_VULKAN_GLSL
<FrameLights block IF file references ANY FrameLights member>
<FrameAtmosphere block IF file references ANY FrameAtmosphere member>
#else
uniform <type> <member1>;
uniform <type> <member2>;
...
#endif
```

**A2 で確立した拡張点 (A1 範式追加)**:
- **conditional injection**: file が UBO member を 1 件でも参照する場合のみ block 注入。両 block 参照 file は同一 `#ifdef LL_VULKAN_GLSL` 内に多重 block 同居可
- **non-canonical uniform 残置**: primary cluster 内に存在するが canonical member ではない uniform (例: cloudsV/skyV の `haze_horizon` / `cloud_shadow` / `sun_moon_glow_factor`) は swap 外へ move して別位置に再挿入、GL 機能変化 0 (5 deletion 例外、`#else` 分岐の byte-for-byte 維持精神は維持)
- **isolated A2 line `#ifndef LL_VULKAN_GLSL` wrap**: primary group 外の散在 member 宣言 = `#ifndef LL_VULKAN_GLSL ... #endif` で条件付き skip (A1 範式継承、A2 で 14 occurrence)

### §2.4 hard rule 5 件 (Agent prompt 必須注入、A1 範式継承)

1. **UBO declaration byte-for-byte canonical**: §2.1 + §2.2 literal を全 file で完全一致コピー、type/member name/order 改変禁止
2. **3-段 swap pattern 厳守**: `#ifdef LL_VULKAN_GLSL ... #else ... #endif` の三段、`#if defined(LL_VULKAN_GLSL)` 等変形禁止
3. **binding 番号 §3 表遵守**: A3 = set=0/binding=3-21 (FrameSamplers 19 件、binding rule artifact §2 (D) 参照)、独自割当禁止
4. **1 file 1 patch**: primary group + isolated wraps を 1 commit 内、ファイル横断 share/共通化禁止
5. **skip list 15 file 機械的 exclusion** (A1 13 + A2 拡張 2): `bundle-A-skip-list.txt` 13 file + A2 拡張 2 file (本 doc §2.5)

### §2.5 A2 拡張 skip list 2 file (15 file 体制)

- `class1/objects/previewV.glsl` — `ambient_color vec4` (vs 他 vec3) + `light_attenuation vec3[8]` (vs 他 vec4[8]) で **2 軸 type divergence**、FS:Beq 改変経路の可能性、後段 sub-bundle で個別 canonical 再設計
- `class3/deferred/multiPointLightF.glsl` — `light[LIGHT_COUNT]` + `light_col[LIGHT_COUNT]` の **LIGHT_COUNT define 依存** + per-pass local light で frame-global 不適、後段 sub-bundle で個別 binding (PerPassLights UBO 等) 検討

bundle-A-skip-list.txt は **本 commit で未更新** = 15 file 体制は本 handoff doc を A3+ source-of-truth として優先、bundle-A-skip-list.txt 自体への 2 file 追記は A3 prep 時に AYA 承認下で実施可能性あり

## §3 A3 着手境界 (次 sub-bundle 設計)

### §3.1 A3 scope

- **FrameSamplers** binding (D) set=0/binding=3-21 = 19 sampler 直接 binding (binding rule artifact §2 (D) 参照)
  - depthMap (3) / lightMap (4) / lightFunc (5) / environmentMap (6) / reflectionProbes (7) / shadowMap0-5 (8-13) / noiseMap (14) / cloud_noise_texture (15) / cloud_noise_texture_next (16) / glowNoiseMap (17) / sceneMap (18) / sceneDepth (19) / brdfLut (20) / exposureMap (21)
- **設計差分**: A1/A2 は UBO ブロック化、A3 は **individual sampler declaration** = `layout(set=0, binding=M) uniform sampler2D <name>;` を直接注入。UBO block を作らない、3-段 swap の `#ifdef LL_VULKAN_GLSL` 内側は sampler 列のみ。
- 推定 file 数 = **~30 file** (bundle-A-prep `1434341904` §3.1 estimate、A3-trace で実数確定 = A1/A2 同様 estimate 楽観可能性あり)
- 推定 Agent 並列 = **2 Agent**

### §3.2 environmentMap dual-type 注意

- `environmentMap` は sampler2D (5 file) と samplerCube (4 file) の **dual-type**
- 同 binding 番号 (set=0/binding=6) を共有するが、Vulkan は per-program 別 type で個別解決可能 (program 毎に descriptor set layout 独立)
- A3 patch では file 毎に actual type を grep で判定して正しい型を注入

### §3.3 A3 着手前チェックリスト 10 件

1. `git fetch && git status` で HEAD = `9f77f875db` 確認
2. AYAstorm 改変 11 file リスト確定 (skip list base 13 - exemplar 2):
   - Picker 2: `class1/deferred/fsObjectID{V,F}.glsl`
   - Cinematic BD 2: `cinematic_bd/class1/deferred/shadowUtil.glsl` + `cinematic_bd/class3/deferred/screenSpaceReflUtil.glsl`
   - Visual Realism 7: `class1/deferred/godrays{V,F}.glsl` + `class1/deferred/volumetricLightF.glsl` + `class3/deferred/volumetricLightF.glsl` + `class1/deferred/blurLight{V,F}.glsl` + `class1/windlight/atmosphericsFuncs.glsl`
3. skip list 15 file 体制 (本 A2-complete §2.5 で 2 file 追加 = previewV + multiPointLightF) を A3 Agent prompt に注入
4. binding rule artifact = `/tmp/bundle-A-binding-rules.md` (本 A2 session で §2 (B) は更新せず、A3 で §2 (D) を再読み込み、artifact 自体は /tmp persist 性 fragile)
5. bundle-A-prep doc 読了: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md` (commit `1434341904`)
6. β-2-hook complete doc 読了: `handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md` (commit `cecb9e6467`)
7. A1 complete doc 読了: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md`
8. 本 A2 complete doc 読了 (この doc、特に §2.3 conditional injection + non-canonical uniform 残置 範式)
9. project memory 確認: `project_ayastorm_r41_vulkan_migration.md` (A2 完遂 + A3 着手境界 active 状態)
10. A3-trace 着手前に AYA 明示 「OK」承認待ち (feedback_one_step_at_a_time / feedback_no_auto_commit)

### §3.4 推奨 cadence 8 step (A1/A2 範式継承)

1. **A3-trace**: Claude が 19 sampler 全件 grep + union 構築 + skip list 15 file 機械的除外 + environmentMap type 判定 + Agent prompt 分割 → AYA 確認
2. **A3-prep**: Claude が 2 Agent prompt 構築 (binding rule artifact §2 (D) + skip list 15 file + individual sampler 3-段 swap sample 注入) → AYA 確認
3. **A3-patch**: Agent 2 並列 実行、AYA 明示 「OK」承認下で起動
4. **A3-verify (self)**: Claude が patch 結果 self-verify (skip list 違反 0 + sampler binding 番号 §2 (D) 完全一致 + LL_VULKAN_GLSL 出現件数集計)
5. **A3-handoff to AYA**: Claude が cache 完全 clear (`rm -rf ~/.ayastorm_x64/cache/`) + shader cp + AYA launch + clean shutdown 確認依頼
6. **A3 measurement**: Claude が AYA launch 後 log 解析で hook fire + parse fail breakdown 取得 (期待 = binding error 37 → 大幅減少観測可能性あり、A1/A2 比 measurement 真の有意差を期待できる sub-bundle = bundle-B 着手前の最終 metric)
7. **A3-commit**: Claude が verification log 除去 + AYA 明示 「OK commit して」承認下で 1 commit (A1/A2 範式継承、Co-Authored-By: Claude なし)
8. **A3-complete handoff doc 起草**: 本 A2 complete doc 範式継承で A3 complete doc 起草

## §4 risks/caveats 10 件

1. **measurement plan §6.1 楽観性再是正**: A2 段階で確認 = glslang **per-file first-error fail semantics** により、A2 patch そのものは効いているが metric では非常に控えめ (non-opaque -1 / location +1 = net 0)。A3 で binding 37 件は大幅減少観測可能性あり (sampler binding が sole error の file が複数存在する可能性) だが、bundle-A 全体 (A1-A6) 完遂時の集合的閾値で評価
2. **cold cache launch verify 必須**: A1/A2 で確認済、A3 verify 前 `rm -rf ~/.ayastorm_x64/cache/` 必須、prior `.shaderbin` GL binary cache hit で β-2-hook bypass 確認済 (A1 session で実証)
3. **glslang missing #endif cascade artifact**: A1 比 A2 で件数変化なし (8 → 8)、bundle-A 全体完遂で根本 error 解消時に自然消滅
4. **conditional injection 範式 (A2 確立)**: A3 では sampler 個別宣言なので file が sampler を 1 件でも参照する場合のみ binding qualifier 注入、A2 の FrameLights/FrameAtmosphere conditional 範式継承
5. **non-canonical uniform 残置 (A2 例外パターン)**: cloudsV/skyV の 5 deletion 例外 = primary cluster 内の non-canonical uniform を swap 外へ move して別位置に再挿入、GL 機能変化 0、`#else` 分岐 byte-for-byte 維持精神は守られている。A3 でも同様 case あり得る (sampler cluster 内に非 §2 (D) sampler が混在する file の可能性)
6. **environmentMap dual-type**: A3 で per-program type 判定 (file 毎に sampler2D / samplerCube grep) 必須、同 binding 番号 set=0/binding=6 共有可
7. **binding rule artifact persist 性**: `/tmp/bundle-A-binding-rules.md` は /tmp persist 性 fragile (fresh OS reboot で失効)、A3 session 開始時に artifact 存在確認 + 失効時は bundle-A-prep `1434341904` §2 表から再生成 (または本 doc §2.1 + §2.2 範式で member 列を逆推定)
8. **AYAstorm 改変 11 file + A2 拡張 2 file untouched 維持**: A3 でも skip list 15 file 体制 (base 13 + A2 拡張 2) 機械的 exclusion 遵守、Agent prompt に必須注入
9. **exemplar 2 (diffuseV/F) untouched 維持**: β-1 PoC 試作レール (sub-doc 03 §3.1.3) として固定、本 A3 でも untouched
10. **context budget concern**: A3 session 内で trace + prep + 2 Agent 並列 patch + self-verify + AYA handoff + measurement + commit + handoff doc の 8 step 全部、fresh context 推奨 (A1/A2 session で残量 fragmenting 確認、sub-bundle 単位 session 分割 cadence の妥当性確認済)

## §5 AYA 承認境界 6 件

1. 本 A2-complete handoff doc commit (doc-only commit、AYA 確認下)
2. A3 着手指示 (明示 「OK」必要、feedback_no_auto_commit + feedback_one_step_at_a_time)
3. A3-patch Agent 並列起動 (Agent 2 並列推定、AYA 確認下)
4. A3 commit (AYA 「OK commit して」明示指示下のみ)
5. 11 file + A2 拡張 2 + exemplar 2 = 15 file untouched 境界
6. canonical 補正方針継承 (A1/A2 で確立、actual GLSL types 優先で body code 不変担保、A3 でも sampler type は actual に合わせる)

## §6 次 session 投入 prompt (fresh context 推奨)

以下を fresh context の次 Claude session に投入してください:

```
sub-step 4.3-γ'-port-β-2-bundle-A-A3 (per-frame frame-global sampler binding 注入) 着手お願いします。

【前提】
- HEAD: 9f77f875db on feature/ayastorm-r41-gl-removal
- 前段: β-2-hook fcf2b6c508 → bundle-A-prep 1434341904 → bundle-A-A1 6f941c0a48 → bundle-A-A2 9f77f875db 完遂

【読了必須】
1. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A2-complete.md (本 handoff doc、A2 範式 + A3 着手境界 + 推奨 cadence 8 step + risks 10 件)
2. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md (A1 範式の上位継承)
3. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md (commit 1434341904、bundle-A 全体設計 §2 binding 表 + sub-bundle A1-A7 cadence)
4. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md (commit cecb9e6467、β-2-hook = per-program SPIR-V hook 設計)
5. docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt (13 file 機械的 exclusion source-of-truth、A2 拡張 2 file は本 A2-complete doc §2.5 参照)
6. memory: project_ayastorm_r41_vulkan_migration.md (A2 完遂 / A3 着手境界 active)
7. /tmp/bundle-A-binding-rules.md (存在確認 + §2 (D) FrameSamplers 19 sampler 表を A3 注入 source-of-truth として再読み込み、/tmp 失効時は bundle-A-prep §2 表から再生成)

【A3 scope】
- FrameSamplers binding (D) set=0/binding=3-21 = 19 sampler 直接 binding (binding rule artifact §2 (D) 参照)
- 設計差分: A1/A2 UBO ブロック化 → A3 individual sampler declaration、layout(set=0, binding=M) uniform sampler2D <name>; 直接注入
- environmentMap dual-type (sampler2D 5 file + samplerCube 4 file) per-program 判定必須
- 推定 file 数 ~30 file 2 Agent 並列 (A3-trace で実数確定、prep estimate 楽観可能性は A1/A2 で実証済)

【cadence】(A1/A2 範式継承 8 step、handoff doc §3.4)
1. A3-trace (Claude grep + union + skip list 15 file 機械的除外 + Agent prompt 分割 → AYA 確認)
2. A3-prep (2 Agent prompt 構築 → AYA 確認)
3. A3-patch (Agent 並列、AYA 明示 「OK」起動)
4. A3-verify (Claude self-verify)
5. A3-handoff (cache clear `rm -rf ~/.ayastorm_x64/cache/` + shader cp + AYA launch verify)
6. A3 measurement (Claude log 解析)
7. A3-commit (AYA 「OK commit して」明示下)
8. A3 complete handoff doc 起草

【hard rule 5 件】(handoff doc §2.4)
1. UBO/sampler declaration byte-for-byte canonical
2. 3-段 swap pattern (#ifdef LL_VULKAN_GLSL ... #else ... #endif) 厳守
3. binding 番号 §2 (D) 表遵守 (depthMap=3 から exposureMap=21 まで)
4. 1 file 1 patch、横断 share 禁止
5. skip list 15 file 機械的 exclusion (base 13 + A2 拡張 2 = previewV + multiPointLightF)

【feedback rule 10 件】(全 commit で遵守)
- feedback_doubt_self_first / feedback_no_scope_shrink / feedback_one_step_at_a_time / feedback_use_agents_proactively / feedback_self_verify_before_handoff / feedback_remove_verification_logs / feedback_no_claude_coauthor / feedback_no_auto_commit (AYA 「OK」明示下のみ commit) / feedback_proactive_handoff / feedback_explanation_lead_with_conclusion

【1st action】
A3-trace 着手前に AYA に明示「OK」確認。trace の前に上記 7 件読了済を 1 行 status で報告。
```

## §7 cross reference

- **handoff doc**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md` (A1 範式上位継承) / `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md` (commit `1434341904`) / `handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md` (commit `cecb9e6467`)
- **spec sub-doc**: 06 §1.2.2/§1.2.4/§3.1 sub-step 6.3 / 07 §3.1 sub-step 7.2-7.4
- **charter**: §3 #1 + §7.5
- **memory**: `project_ayastorm_r41_vulkan_migration.md` (γ'-port-β-2-bundle-A-A2 完遂 + γ'-port-β-2-bundle-A-A3 着手境界 active)
- **artifact**: `docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt` (repo 内 base 13 file、A2 拡張 2 file は本 doc §2.5 source-of-truth) / `/tmp/bundle-A-binding-rules.md` (/tmp 配置、失効性あり、§2 (B) は本 doc §2.1 補正後 canonical を A3+ 優先)
- **feedback rules**: doubt_self_first / no_scope_shrink / one_step_at_a_time / use_agents_proactively / self_verify_before_handoff / remove_verification_logs / no_claude_coauthor / no_auto_commit / proactive_handoff / explanation_lead_with_conclusion

---

→ 次 action = AYA review + 本 handoff doc commit (doc-only、AYA 「OK」明示下) → fresh context で A3 着手 (上記 §6 prompt 投入)
