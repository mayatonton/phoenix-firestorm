# r41 sub-step 4.3-γ'-port-β-2-bundle-A-A3 完遂 → A4 着手境界 handoff (2026-06-01)

**parent commit**: `ebd5e2b16d` (A3 patch、本 handoff の直接 parent) / `ac3f294643` (A8-recovery-complete handoff、A3 着手境界 source-of-truth)
**HEAD**: `ebd5e2b16d` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: A3 完遂状態 + A4 着手境界 を fresh context 引継 用に確定する doc-only handoff。AYA 「OK」明示承認下で commit (no auto-commit)。

---

## §1 A3 完遂 status

| 項目 | 値 |
|---|---|
| Scope | per-frame frame-global sampler binding qualifier (D) set=0/binding=3-21 = 19 sampler 個別 `layout(set=0, binding=M) uniform <type> <name>;` 注入 |
| 注入 file 数 | **32 file** (Agent1 18 = class1 系 + Agent2 14 = class2 + class3 + cinematic_bd) |
| 変更行数 | **+163 / -0** (insertions-only、A1/A2 のような non-canonical uniform move 例外なし) |
| sampler 注入 declaration 総数 | **52 件** (binding 番号別 §1.3 表) |
| 3-段 swap block 数 | **37 block** (32 file × 1+、複数 cluster 5 file = deferredUtil/shadowUtil/cinematic_bd shadowUtil/class3 reflectionProbeF/softenLightF) |
| A1/A2/A8 既処理 UBO untouched | **5 file 全て UBO 行 diff = 0** (class3/volumetricLightF + class1/blurLightF + class1/shadowUtil + cinematic_bd shadowUtil + cinematic_bd screenSpaceReflUtil) |
| skip list 4 file untouched | **全 untouched** (exemplar 2 = diffuseV/F + A2 拡張 2 = previewV/multiPointLightF) |
| AYA cold cache launch verify | **PASS** (起動成立 + clean shutdown + GL .shaderbin 223 再生成 + crash 0 + GL shader compile/link fail 0) |
| commit | `ebd5e2b16d` (AYA 「OK commit して」明示指示下) |

### §1.1 binding rule §2 (D) literal (canonical、A3 で確定)

```
| sampler                  | type             | binding              |
|--------------------------|------------------|----------------------|
| depthMap                 | sampler2D        | set=0, binding=3     |
| lightMap                 | sampler2D        | set=0, binding=4     |
| lightFunc                | sampler2D        | set=0, binding=5     |
| environmentMap           | sampler2D OR     | set=0, binding=6     |
|                          | samplerCube      | (file 毎 actual type) |
| reflectionProbes         | samplerCubeArray | set=0, binding=7     |
| shadowMap0               | sampler2DShadow  | set=0, binding=8     |
| shadowMap1               | sampler2DShadow  | set=0, binding=9     |
| shadowMap2               | sampler2DShadow  | set=0, binding=10    |
| shadowMap3               | sampler2DShadow  | set=0, binding=11    |
| shadowMap4               | sampler2DShadow  | set=0, binding=12    |
| shadowMap5               | sampler2DShadow  | set=0, binding=13    |
| noiseMap                 | sampler2D        | set=0, binding=14    |
| cloud_noise_texture      | sampler2D        | set=0, binding=15    |
| cloud_noise_texture_next | sampler2D        | set=0, binding=16    |
| glowNoiseMap             | sampler2D        | set=0, binding=17    |
| sceneMap                 | sampler2D        | set=0, binding=18    |
| sceneDepth               | sampler2D        | set=0, binding=19    |
| brdfLut                  | sampler2D        | set=0, binding=20    |
| exposureMap              | sampler2D        | set=0, binding=21    |
```

### §1.2 handoff `ac3f294643` §4.1 environmentMap 記述補正 (本 A3 で確定)

handoff 元記述「sampler2D 5 file + samplerCube 4 file」は誤記。実 grep + binding rule artifact §3.4 が source-of-truth:
- sampler2D = **1 file** (`class1/deferred/skyF.glsl` のみ)
- samplerCube = **4 file** (`class2/deferred/reflectionProbeF` + `class3/deferred/materialF` + `class3/deferred/fullbrightShinyF` + `class3/deferred/spotLightF`)
- total = **5 declaration**

同 binding `set=0, binding=6` 共有、Vulkan per-program descriptor set layout で個別解決 (file 毎 actual type 注入)。

### §1.3 binding 番号別 注入 count (32 file 内 52 declaration の内訳)

| binding | sampler | count |
|---|---|---|
| 3 | depthMap | 10 |
| 4 | lightMap | 6 |
| 5 | lightFunc | 4 |
| 6 | environmentMap (dual-type) | 5 (sampler2D 1 + samplerCube 4) |
| 7 | reflectionProbes | 3 |
| 8 | shadowMap0 | 2 |
| 9 | shadowMap1 | 2 |
| 10 | shadowMap2 | 2 |
| 11 | shadowMap3 | 2 |
| 12 | shadowMap4 | 2 |
| 13 | shadowMap5 | 2 |
| 14 | noiseMap | 1 |
| 15 | cloud_noise_texture | 1 |
| 16 | cloud_noise_texture_next | 1 |
| 17 | glowNoiseMap | 1 |
| 18 | sceneMap | 3 |
| 19 | sceneDepth | 2 |
| 20 | brdfLut | 1 |
| 21 | exposureMap | 2 |
| **合計** | | **52** |

### §1.4 cold cache launch verify metric (2026-06-01) vs A8-recovery baseline

| metric | A8-recovery baseline | A3 actual | delta |
|---|---|---|---|
| β-2-hook fire (`generatePerProgramSPIRV`) | 224 | 224 | 0 |
| parse failed (per-program) | 224/224 = 100% fail | 224/224 = 100% fail | 0 (controlled) |
| location error | 152 | 152 | 0 |
| binding error | 37 | 37 | 0 |
| non-opaque uniforms outside block | 35 | 35 | 0 |
| missing #endif | 8 | 8 | 0 |
| SPIR-V 生成成功 | 0 | 0 | 0 (controlled) |
| GL `.shaderbin` 再生成 | 223 | 223 | 0 (GL regression 0) |
| GL compile/link fail | 0 | 0 | 0 |
| crash (真) | 0 (mention 全 benign) | 0 (mention 4 件全 benign = `settings_crash_behavior.xml` load 3 + save 1) | 0 |
| clean shutdown | OK | OK (Goodbye! + Vulkan device destroyed + Vulkan instance destroyed) | OK |

**評価**: A2/A8 同様 controlled measurement、metric 不変は prep doc §1 / A8-recovery handoff §8.1 で予測通り。glslang per-file first-error fail semantics により同 file 内 location error (152) が先 fire、binding error (37) は同 file 内で blocked。bundle-A 全体完遂時の集合的閾値で評価する設計 (bundle-A-prep §6.1: A1-A6 完遂で parse pass 率 ~95% 期待)。

charter §3 #1 acceptance 担保 (GL path untouched + 起動成立 + clean shutdown + GL regression 0 + AYAstorm 独自改造機能特性そのまま)。

---

## §2 A3 で確定した範式 (A4-A7 継承必須)

### §2.1 設計差分 (A1/A2/A8 と A3)

- A1/A2/A8 = **UBO block 化** = `layout(set=N, binding=M, std140) uniform <BlockName> { ... };`
- A3 = **individual sampler declaration** = `layout(set=0, binding=M) uniform <type> <name>;` 直接注入、UBO block 不作
- A4 以降 (per-material) = UBO block (MaterialUBO) + individual sampler (Diffuse/Normal/Spec) 混在予定 = A1/A2/A8 + A3 範式併用

### §2.2 3-段 swap pattern (A1/A2/A8 範式継承 + A3 sampler 適用)

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=0, binding=M) uniform <type> <name>;
#else
uniform <type> <name>;
#endif
```

**連続 sampler cluster は同一 swap 内 collect**、gap (空行/他 uniform) で分断された別 cluster は別 swap block。

### §2.3 conditional injection 範式 (A2/A8 共通、A3 で確認)

file が当該 sampler 1 件でも参照する場合のみ binding qualifier 注入。参照 0 件 file は touch 無し (自動 skip = exemplar 2 + previewV = 3 file)。

### §2.4 既処理 file 範式 (A1/A2/A8 既処理 file の UBO untouched)

A3 で 5 file (class3/volumetricLightF + class1/blurLightF + class1/shadowUtil + cinematic_bd shadowUtil + cinematic_bd screenSpaceReflUtil) で UBO `layout(set=N, binding=M, std140) uniform FrameXxx { ... };` block を **絶対 touch しない** rule を Agent prompt enforce で違反 0 件確認。A4 以降も既処理 5 file の UBO block + A3 で注入した sampler binding qualifier (合計 12 declaration、§1.3) は untouched 継続。

### §2.5 environmentMap dual-type 範式 (A3 で確定)

同 binding `set=0, binding=6` 共有、Vulkan per-program descriptor set layout で個別解決。file 毎 actual type を `uniform\s+(sampler2D|samplerCube)\s+environmentMap` grep で判定し、actual type を注入。A3 で 5 file 全件 type mismatch 0 件確認。同様の dual-type sampler が A4-A5 で出現する場合は per-file actual type 判定範式継承。

### §2.6 hard rule 6 件 (A4-A7 Agent prompt 必須注入)

1. **sampler/UBO declaration byte-for-byte canonical** (binding 番号 + type は binding rule artifact 表遵守、actual GL declarator と type 一致)
2. **3-段 swap pattern 厳守** (`#ifdef LL_VULKAN_GLSL ... #else ... #endif` 三段、`#if defined(LL_VULKAN_GLSL)` 等変形禁止)
3. **binding 番号 binding rule artifact 表遵守** (独自割当禁止)
4. **1 file 1 patch** (file 横断 share/共通化禁止)
5. **A1/A2/A3/A8 既処理 file の UBO block + sampler binding qualifier 再 touch 禁止** (A4 以降では新規 sampler/uniform binding のみ追加)
6. **dual-type sampler/uniform は file 毎 actual type per-file grep 判定** (environmentMap A3 範式継承)

---

## §3 A4 着手境界

### §3.1 A4 scope (bundle-A-prep §3.1)

| 項目 | 値 |
|---|---|
| Scope | per-material core uniform/sampler |
| binding | (E) MaterialUBO + (F) Diffuse/Normal/Spec sampler binding |
| key entities | `texture_matrix0` (39 file 参照) + `diffuseMap` (41 file 参照) overlap |
| 推定 file 数 | **~60 unique file** |
| Agent 並列数 | **4 Agent** (class1/deferred + class1/objects + interface + gltf 分割) |
| 工期 | ~半日-1 日 (Agent 並列で短縮) |

### §3.2 A4 着手前チェックリスト (10 件)

1. `git fetch origin` 実施
2. HEAD = `ebd5e2b16d` 確認 (本 A3 patch commit)
3. AYAstorm 独自改造 11 file (A8-recovery で全 UBO 注入完了 + 本 A3 で sampler binding 注入完了) の touch 状態 = 通常 file と同等扱い
4. skip list 4 file (exemplar 2 = diffuseV/F + A2 拡張 2 = previewV/multiPointLightF) 機械的 exclusion 継続
5. binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (E) MaterialUBO + (F) sampler binding 再読み込み (`/tmp` persist 性 fragile = 失効時は bundle-A-prep `1434341904` §2 表 + 本 doc §1.1 範式から再生成)
6. 5 件 prior handoff doc 読了 (A1-complete `99afb8f1bc` + A2-complete `8e69841a53` + A8-recovery-complete `ac3f294643` + bundle-A-prep `1434341904` + 本 A3-complete)
7. project memory `project_ayastorm_r41_vulkan_migration.md` 確認 (γ'-port-β-2-bundle-A-A3 完遂 + γ'-port-β-2-bundle-A-A4 着手境界 active)
8. A1/A2/A3/A8 既処理 file (94 + 52 + 32 + 8 = 重複あり、union ~150 file 程度) の UBO block + sampler binding qualifier 再 touch 禁止 rule の Agent prompt enforce
9. cold cache launch verify 準備 (`rm -rf ~/.ayastorm_x64/cache/` 必須)
10. AYA 「OK」承認待ち (A4-trace 着手前)

### §3.3 推奨 cadence 8 step (A1/A2/A3/A8 範式継承)

1. **A4-trace**: Claude grep + union + skip list 機械的除外 + dual-type sampler/uniform per-file actual type 判定 + 4 Agent prompt 分割
2. **A4-prep**: 4 Agent prompt 構築 (binding rule §2 (E)+(F) literal + hard rule 6 件 + 3-段 swap pattern sample + skip list 4 file + 既処理 file 警告 list + assigned file list + 実装手順 + 完遂報告 format)
3. **A4-patch**: AYA「OK」明示下 4 Agent 並列起動
4. **A4-verify**: Claude self-verify (patched file count + skip 違反 + 既処理 UBO/sampler untouched + canonical byte-for-byte + 3-段 swap 整合 + binding 番号集計 + insertions-only)
5. **A4-handoff**: shader 全件 `~/ayastorm/app_settings/shaders/` cp + `rm -rf ~/.ayastorm_x64/cache/` 実施 + AYA cold cache launch verify 依頼 (clean shutdown 確認)
6. **A4-measurement**: Claude log 解析 (期待 location error 152 件大幅減少観測可能性あり = A4 per-material 解消で同 file 内 location qualifier 不足が露出 → bundle-A 全体閾値で再確認)
7. **A4-commit**: AYA 「OK commit して」明示指示下のみ
8. **A4-complete handoff doc 起草**: 本 doc 範式継承

---

## §4 risks/caveats 8 件

### §4.1 measurement plan 是正済継承

A2/A8/A3 で確認した glslang per-file first-error fail semantics により、sub-bundle 単独 metric は controlled で baseline 同等。bundle-A 全体 (A1-A6) 完遂時の集合的閾値で評価。

### §4.2 cold cache launch verify 必須

`rm -rf ~/.ayastorm_x64/cache/` を A4 verify 前必須化。prior `.shaderbin` cache hit で β-2-hook bypass する経路は β-2-hook complete 検証で確認済。

### §4.3 既処理 file 5 件 (A1/A2/A8) + A3 で sampler 注入 = 一体不可分

A4 以降では既処理 5 file の UBO block + A3 で注入した sampler binding qualifier (合計 12 declaration) を **絶対 touch しない**、新規 (E) MaterialUBO + (F) sampler 注入のみ追加。Agent prompt enforce 必須。

### §4.4 environmentMap dual-type 範式 (A4 dual-type 出現時継承)

A3 で確立した per-file actual type 判定範式 = `uniform\s+(<type1>|<type2>)\s+<name>` grep で actual 確認。type mismatch 時は FAIL 報告で次 file (A3 では type mismatch 0 件)。

### §4.5 4 Agent 並列の coordination

A4 は 4 Agent 並列 (A1 = 4 / A2 = 2 / A3 = 2 推移)。Agent prompt 内 assigned file list は重複なしで union 形成、skip list 4 file + 既処理 file 警告 list (A1/A2/A3/A8 既処理 + A3 で sampler 注入) を全 Agent に共通注入。

### §4.6 binding rule artifact `/tmp` persist 性 fragile

`/tmp/bundle-A-binding-rules.md` (558 line / 35KB) は fresh OS reboot で失効。A4 session 開始時に artifact 存在確認 + 失効時は bundle-A-prep `1434341904` §2 表 + handoff `ac3f294643` §4.1 (補正後) + 本 doc §1.1 + commit message §2 から再生成。

### §4.7 残 skip 既知 list (A4 でも継承)

- exemplar 2 (`diffuseV/F`) untouched 維持 (sub-doc 03 §3.1.3 β-1 PoC 試作レール)
- A2 拡張 skip 2 (`previewV/multiPointLightF`) untouched 維持 (後段 sub-bundle で個別対応)

### §4.8 context budget concern

A4 session 内で trace + prep + 4 Agent 並列 patch + self-verify + AYA handoff + measurement + commit + handoff doc の 8 step 全部は fresh context 推奨 (A1 = 4 Agent / A2 = 2 Agent / A3 = 2 Agent で実証済の session 分割 cadence)。

---

## §5 AYA 承認境界 6 件

1. 本 A3-complete handoff doc commit (AYA 「OK commit して」明示指示下のみ)
2. A4 着手指示 (AYA 「OK」明示要)
3. A4-patch 4 Agent 並列起動 (AYA 「OK」明示要)
4. A4 commit (AYA 「OK commit して」明示指示下のみ)
5. A1/A2/A3/A8 既処理 file (UBO block + sampler binding qualifier) 再 touch 禁止境界
6. skip list 4 file (exemplar 2 + A2 拡張 2) untouched 継続

---

## §6 次 session 投入 prompt (fresh context 推奨)

```
AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-A-A4 着手。

[読了必須 8 件]
1. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A3-complete.md (本 handoff)
2. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A8-recovery-complete.md (ac3f294643)
3. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A2-complete.md (8e69841a53)
4. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md (99afb8f1bc)
5. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md (1434341904)
6. /tmp/bundle-A-binding-rules.md §2 (E) MaterialUBO + (F) Diffuse/Normal/Spec sampler binding (persist 性 fragile = 失効時は bundle-A-prep §2 + 本 A3-complete §1.1 から再生成)
7. ~/.claude/projects/-home-ishikawa-work-firestorm-phoenix-firestorm/memory/project_ayastorm_r41_vulkan_migration.md
8. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md (cecb9e6467、β-2-hook 範式)

[A4 scope]
- per-material core uniform/sampler (texture_matrix0 39 file + diffuseMap 41 file overlap)
- 推定 ~60 unique file、4 Agent 並列
- (E) MaterialUBO + (F) Diffuse/Normal/Spec sampler binding

[cadence 8 step]
A4-trace → A4-prep → A4-patch (AYA 明示 OK 要) → A4-verify (self) → A4-handoff (shader cp + rm -rf cache/ + AYA launch verify) → A4-measurement → A4-commit (AYA 「OK commit して」明示要) → A4-complete handoff doc 起草

[hard rule 6 件]
1. sampler/UBO declaration byte-for-byte canonical (binding rule artifact 表遵守)
2. 3-段 swap pattern 厳守 (#ifdef LL_VULKAN_GLSL ... #else ... #endif)
3. binding 番号 binding rule artifact 表遵守 (独自割当禁止)
4. 1 file 1 patch (file 横断 share/共通化禁止)
5. A1/A2/A3/A8 既処理 file の UBO block + sampler binding qualifier 再 touch 禁止
6. dual-type sampler/uniform は file 毎 actual type per-file grep 判定

[feedback rule 12 件]
- doubt_self_first / no_scope_shrink / self_verify_before_handoff / use_agents_proactively / admit_unknown / falsification_as_progress / explanation_lead_with_conclusion / no_claude_coauthor / no_auto_commit / one_step_at_a_time / proactive_handoff / remove_verification_logs

[1st action]
A4-trace 着手前に AYA 「OK」確認。読了済 1 行 status を AYA に報告。
```

---

## §7 cross reference

- handoff `99afb8f1bc` (A1-complete)
- handoff `8e69841a53` (A2-complete)
- handoff `ac3f294643` (A8-recovery-complete、A3 着手境界 source-of-truth)
- handoff `1434341904` (bundle-A-prep、sub-bundle A1-A7 cadence + skip list 機械的 exclusion)
- handoff `cecb9e6467` (β-2-hook-complete、per-program SPIR-V hook 範式)
- commit `6f941c0a48` (A1 patch、94 file)
- commit `9f77f875db` (A2 patch、52 file)
- commit `aed1438936` (A8-recovery patch、8 file)
- commit `ebd5e2b16d` (A3 patch、32 file、本 handoff の直接 parent)
- sub-doc 06 §1.2.2/§1.2.4/§3.1 sub-step 6.3
- sub-doc 07 §3.1 sub-step 7.2-7.4
- sub-doc 03 §3.1.3 (exemplar 2 役割)
- charter §3 #1 + §7.5
- binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (D) (本 A3 で確定) + §2 (E)+(F) (A4 着手 source-of-truth)
- project memory `project_ayastorm_r41_vulkan_migration.md` (γ'-port-β-2-bundle-A-A3 完遂 + γ'-port-β-2-bundle-A-A4 着手境界 active)
- skip list `docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt` (base + A2 拡張)
- feedback rules: feedback_doubt_self_first / feedback_no_scope_shrink / feedback_self_verify_before_handoff / feedback_use_agents_proactively / feedback_admit_unknown / feedback_falsification_as_progress / feedback_explanation_lead_with_conclusion / feedback_no_claude_coauthor / feedback_no_auto_commit / feedback_one_step_at_a_time / feedback_proactive_handoff / feedback_remove_verification_logs
