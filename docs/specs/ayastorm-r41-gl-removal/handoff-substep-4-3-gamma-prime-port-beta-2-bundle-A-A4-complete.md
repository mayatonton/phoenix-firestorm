# r41 sub-step 4.3-γ'-port-β-2-bundle-A-A4 完遂 → A5 着手境界 handoff (2026-06-01)

**parent commit**: `f703710f8d` (A4 patch、本 handoff の直接 parent) / `524391d78d` (A3-complete handoff、A4 着手境界 source-of-truth だった doc)
**HEAD**: `f703710f8d` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: A4 完遂状態 + A5 着手境界 を fresh context 引継 用に確定する doc-only handoff。AYA 「OK」明示承認下で commit (no auto-commit)。

---

## §1 A4 完遂 status

| 項目 | 値 |
|---|---|
| Scope | per-material core uniform/sampler = (E) MaterialUBO `set=1/binding=0/std140` + (F) Diffuse/Normal/Spec sampler `set=1/binding=1-3` |
| 注入 file 数 | **82 file** (4 Agent 並列 = Agent1 class1/deferred V+Util / Agent2 class1/deferred F / Agent3 class1/interface + class1/objects + class1/lighting + class1/effects + class1/avatar / Agent4 class2 + class3 + cinematic_bd + class1/gltf) |
| 変更行数 | **+770 / -0** (insertions-only) |
| MaterialUBO 注入 declaration 総数 | **52 件** (dual IS_HUD branch 4 file × 2 + 単分岐 44 file = 52) |
| sampler 注入 declaration 総数 | diffuseMap binding=1 + normalMap binding=2 + specularMap binding=3 (実際の declaration 数は file 毎 conditional) |
| A1/A2/A3/A8-recovery 既処理 file untouched | **違反 0 件** (Agent prompt enforce) |
| SSBO exclusion | **2 file** (class1/gltf/pbrmetallicroughnessV + F)、UBO 注入 0 件 + sampler のみ注入 |
| dual-context routing 除外 | **3 file** (class3/deferred/pointLightF + spotLightF + class1/deferred/deferredUtil) の `color` UBO 注入 0 件、ただし deferredUtil の normalMap sampler は注入 |
| dual IS_HUD/!IS_HUD branch | **4 file** (pbralphaV/pbropaqueV/pbralphaF/pbropaqueF)、各 file 2 MaterialUBO block |
| skip list 4 file untouched | **全 untouched** (exemplar 2 = diffuseV/F + A2 拡張 2 = previewV/multiPointLightF) |
| AYA cold cache launch verify | **PASS** (起動成立 + clean shutdown + GL .shaderbin 223 再生成 + crash 0 + GL shader compile/link fail 0) |
| commit | `f703710f8d` (AYA 「OK commit して」明示指示下) |

### §1.1 binding rule §2 (E) literal (canonical、A4 で確定 / 補正後)

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=0, std140) uniform MaterialUBO {
    mat4  texture_matrix0;                  // 37 file が参照
    vec4  texture_base_color_transform[2];  //  4 file (PBR uniform decl、pbrmetallicroughnessV/F は SSBO 経由)
    vec4  texture_emissive_transform[2];    //  3 file
    vec4  color;                            // 12 file (material context)
    vec3  emissiveColor;                    //  3 file
    float _pad_emissive;                    // std140 vec3+float 16-byte boundary padding
};
#endif
```

**重要**: A4-trace 段階で binding rule artifact §2 (E) は元記述「`mat4 texture_base_color_transform` / `vec4 emissiveColor`」が実 GLSL grep と不一致 → 自己疑い (feedback_doubt_self_first) → `vec4[2] / vec3` に補正、AYA 明示承認下で確定。member 順序/型/array 次元は本 doc literal を **source-of-truth として byte-for-byte 厳守**。

### §1.2 binding rule §2 (F) literal (per-material sampler 固定 binding、A4 で確定)

```
| sampler      | type      | binding              |
|--------------|-----------|----------------------|
| diffuseMap   | sampler2D | set=1, binding=1     |
| normalMap    | sampler2D | set=1, binding=2     |
| specularMap  | sampler2D | set=1, binding=3     |
```

3-段 swap pattern (A3 範式継承):

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=N) uniform sampler2D <name>;
#else
uniform sampler2D <name>;
#endif
```

連続宣言 cluster は同一 swap block に collect、gap 越えは別 block (A3 範式継承)。

---

## §2 cold cache launch verify metric (2026-06-01) vs A3 baseline

| metric | A3 baseline | A4 current | 差分 | 解釈 |
|---|---|---|---|---|
| hook fire (`generatePerProgramSPIRV`) | 224 | 224 | 0 | cold cache 経路 OK |
| parse fail | 224/224 | 224/224 | 0 | A4 単独 SPIR-V 生成 0% controlled、A1/A2/A3/A8-recovery と同 measurement |
| `ERROR ... 'location'` | 152 | 155 | **+3** | A4 解消の副次効果 cascade |
| `ERROR ... 'binding'` | 37 | 35 | **-2** | A4 patch 直接効果 |
| `ERROR ... 'non-opaque uniforms outside block'` | 35 | 34 | **-1** | A4 patch 直接効果 |
| `ERROR ... missing #endif` | 8 | 8 | 0 | glslang error cascade artifact、bundle-A 全体完遂で自然消滅予定 |
| GL `.shaderbin` 再生成 | 223 | 223 | 0 | GL shader compile/link fail 0、GL path regression 0 |
| crash (真) | 0 | 0 | 0 | 4 件 mention 全 benign (settings_crash_behavior.xml load 3 + save 1) |
| 起動成立 + clean shutdown | OK | OK | - | charter §3 #1 acceptance 担保 |

### §2.1 +3/-3 balance の解釈

`location +3` と `binding -2 + non-opaque -1 = -3` がちょうど balance。これは **A4 patch が 3 file の binding/non-opaque error を解消 → glslang per-file first-error fail semantics により次の location error (bundle-B scope) が露出した cascade**。A4 patch defect ではなく A4 解消の副次効果。controlled measurement design 内 (A2/A3/A8-recovery と同範式)、charter §3 #1 acceptance 担保。bundle-A 全体完遂時の集合的閾値で評価する設計 (bundle-A-prep §6.1)。

### §2.2 PBR shader 群の error 残存

Deferred PBR Opaque/Alpha + HUD PBR + Skinned PBR Shader 群は依然 binding/non-opaque で fail。これは A4 scope 外の SSBO (gltf_material_data) + PerDrawUBO 由来で、**A5 (extension sampler + terrain detail block) + A6 (PerDrawUBO + per-light) 範式で解消予定**。A4 patch 自体は期待通り動作。

---

## §3 A4 で確定した範式 (A5-A7 継承必須)

### §3.1 設計差分の整理

| sub-bundle | UBO/sampler 構造 | 注入形式 |
|---|---|---|
| A1 | (A) FrameViewProj UBO `set=0/binding=0` | UBO block 化 |
| A2 | (B) FrameLights + (C) FrameAtmosphere UBO `set=0/binding=1/2` | UBO block 化 |
| A8-recovery | A1/A2 と同 UBO 範式を AYAstorm 独自改造 11 file へ遅延注入 | UBO block 化 |
| A3 | (D) per-frame frame-global sampler `set=0/binding=3-21` = 19 sampler | individual sampler declaration (UBO block 不作) |
| **A4** | **(E) MaterialUBO `set=1/binding=0/std140` + (F) per-material sampler `set=1/binding=1-3`** | **UBO block + individual sampler 混在 (A1/A2/A8 + A3 範式併用)** |
| A5 (予定) | (F) extension sampler 残 + pbrterrainF emissiveColors[4] terrain detail block | individual sampler + 専用 UBO block |
| A6 (予定) | (G) PerDrawUBO + per-light context (color 等) | UBO block |

### §3.2 KHR_texture_transform packed 範式 (A4 確立)

`texture_base_color_transform vec4[2]` / `texture_emissive_transform vec4[2]`:
- `[0]` = `scale.xy + offset.xy` (4 component)
- `[1]` = `rotation.xy` (sin/cos、2 component + 残 2 component 余白)

GL 実装と Vulkan 実装で型完全一致 → compute identical → 見た目変化最小化 (AYA 明示指示下)。

### §3.3 std140 padding 範式 (A4 確立)

`vec3 emissiveColor` 直後に `float _pad_emissive` を **必須**:
- vec3 は std140 で 16-byte alignment、次の member が float 等 4-byte なら同 16-byte slot に詰めるが、explicit pad で boundary を明示化
- Vulkan SPIR-V/GLSL いずれでも実 layout は同等、defensive padding として canonical literal の一部

### §3.4 dual IS_HUD/!IS_HUD branch 範式 (A4 確立)

`pbralphaV` / `pbropaqueV` / `pbralphaF` / `pbropaqueF` 4 file は `#ifdef IS_HUD ... #else ... #endif` で preprocessor-exclusive 各分岐に MaterialUBO 1 block ずつ独立注入 (計 2 block/file)。preprocessor-exclusive なので 1 program で同時に 2 block 見えることはない (compile time に一方が消える)。

### §3.5 SSBO exclusion 範式 (A4 確立)

SSBO 経由で値取得する file (gltf_material_data 系) は uniform 宣言なし → **UBO member 注入対象外**、sampler のみ (F) binding qualifier 注入対象。
- `class1/gltf/pbrmetallicroughnessV.glsl`
- `class1/gltf/pbrmetallicroughnessF.glsl`

### §3.6 dual-context routing 範式 (A4 確立)

`color` uniform は per-material context (A4 scope) と per-light context (A6 PerDrawUBO scope) で **分離**:
- per-material `color` 注入 → A4 で実施 (12 file)
- per-light `color` 注入 → A6 で実施 (本 A4 では除外):
  - `class3/deferred/pointLightF.glsl`
  - `class3/deferred/spotLightF.glsl`
  - `class1/deferred/deferredUtil.glsl`

ただし `deferredUtil.glsl` は `normalMap` (per-material) を持つので **そっちは (F) で注入対象** (A4 で実施済)。

### §3.7 hard rule 6 件 (A5-A7 Agent prompt 必須注入)

1. **byte-for-byte canonical**: §1.1 MaterialUBO literal + §1.2 sampler swap literal を文字通り copy する。member 順序変更/型変更/追加削除一切禁止。
2. **3-段 swap pattern 厳守**: `#ifdef LL_VULKAN_GLSL ... #else ... #endif`。`#if defined(LL_VULKAN_GLSL)` 等変形禁止。
3. **binding 番号は binding rule artifact §2 表通り**: 独自割当禁止。
4. **1 file 1 patch**: 同 file 内で複数 swap block 必要時 (UBO + sampler 分離) は OK だが、UBO は 1 block に集約 (dual IS_HUD branch 例外を除く)。
5. **A1/A2/A3/A4/A8-recovery 既処理 file の UBO block + sampler binding qualifier 再 touch 禁止**: 違反は即 abort、A5 以降 absolute untouched。
6. **dual-type / dual-branch / SSBO exclusion / dual-context routing は file 毎 actual 判定**: 単純パターンマッチでなく、実 grep + binding rule artifact + 既処理 handoff doc 整合確認後に注入。

---

## §4 A5 着手境界

### §4.1 A5 scope (推定)

| 軸 | 内容 |
|---|---|
| (F) extension sampler 残 | bumpMap / emissiveMap / altDiffuseMap / metallicRoughnessMap / occlusionMap 等 (binding rule artifact §2 (F) 残 entries) |
| terrain detail block | `class1/deferred/pbrterrainF.glsl` の `emissiveColors[4]` 等 per-detail material UBO (専用 block 設計) |
| 推定 file 数 | ~30-40 file (extension sampler 参照広め、terrain は 1-2 file) |
| 推奨 Agent 並列度 | 2-3 Agent (A4 と同方式、file group 分割) |

### §4.2 A5 着手前チェックリスト 10 件

1. `git fetch origin` で origin 最新と同期
2. HEAD = `f703710f8d` 確認 (本 A4 patch commit)
3. AYAstorm 独自改造 11 file は A8-recovery で完遂済 (A4 範式継承通常 file 扱い)
4. skip list 4 file (exemplar 2 + A2 拡張 2) 機械的 exclusion 継続
5. binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (F) extension sampler 残 + terrain detail block セクション再読み込み (失効時は本 doc + A1/A2/A3/A4 完遂 handoff doc から再生成可能)
6. 6 件 prior handoff doc 読了 (本 doc + A3-complete `524391d78d` + A2-complete `8e69841a53` + A1-complete `99afb8f1bc` + A8-recovery-complete `ac3f294643` + bundle-A-prep `1434341904`)
7. 既処理 file 再 touch 禁止 rule (A1/A2/A3/A4/A8-recovery 全 sub-bundle) 絶対 enforce
8. project memory `project_ayastorm_r41_vulkan_migration.md` 確認
9. cold cache launch verify 準備 (`rm -rf ~/.ayastorm_x64/cache/` + shader cp)
10. AYA 「OK」承認待ち (A5-trace 着手前 + A5-patch 起動前 + A5 commit)

### §4.3 推奨 cadence 8 step (A1-A4 範式継承)

1. **A5-trace**: Claude bare entity grep + union + skip list 機械的除外 + dual-type/dual-branch/SSBO 判定 + Agent prompt 分割
2. **A5-prep**: 2-3 Agent prompt 構築 (担当 file list + canonical literal + hard rule 6 件)
3. **A5-patch**: Agent 並列起動 (AYA 「OK」明示指示下、general-purpose subagent_type)
4. **A5-verify**: Claude self-verify (skip list / canonical / 3-段 swap / 既処理 untouched / insertions-only)
5. **A5-handoff**: shader cp `~/ayastorm/app_settings/shaders/...` + `rm -rf ~/.ayastorm_x64/cache/shader_cache/` + AYA launch verify + clean shutdown 確認
6. **A5-measurement**: Claude log 解析 (期待: binding/non-opaque error 減少 + location +N cascade、bundle-A 全体閾値で評価)
7. **A5-commit**: AYA 「OK commit して」明示指示下で commit
8. **A5-complete handoff doc 起草**: A5 完遂状態 + A6 着手境界 を fresh context 引継 用に確定

---

## §5 risks/caveats 8 件

1. **measurement plan 是正済継承**: sub-bundle 単独 metric は controlled で baseline 同等 (location 増減は cascade、bundle-A 全体集合的閾値で評価)。A5 でも binding/non-opaque 大幅減少観測可能性あり、location 連動 +N も予期。
2. **cold cache launch verify 必須**: `rm -rf ~/.ayastorm_x64/cache/` を sub-bundle 毎 verify 前必須化。A4 でも実施済。
3. **既処理 file (A1/A2/A3/A4/A8-recovery) の UBO block + sampler binding qualifier 一体不可分**: A5 以降 absolute untouched。Agent prompt 必須注入で違反 0 件 enforce。
4. **dual-type / dual-branch / SSBO / dual-context routing**: A5 で類似パターン出現時は A4 §3.4-§3.6 範式継承。pbrterrainF terrain detail block は新規 UBO 設計を要する可能性 (A5-trace で確定)。
5. **2-3 Agent 並列 coordination**: assigned file list union 重複なし + skip list + 既処理 file 警告 list 共通注入。
6. **binding rule artifact `/tmp` persist 性 fragile**: 失効時は本 doc §1.1/§1.2 + bundle-A-prep §2 表から再生成可能。
7. **残 skip 既知 list**: exemplar 2 (diffuseV/F) untouched 維持 (sub-doc 03 §3.1.3 β-1 PoC 試作レール) + A2 拡張 skip 2 (previewV/multiPointLightF) untouched 維持。
8. **context budget concern**: A5 session fresh context 推奨。

---

## §6 AYA 承認境界 6 件

1. 本 A4-complete handoff doc commit
2. A5 着手指示
3. A5-patch 2-3 Agent 並列起動
4. A5 commit
5. A1/A2/A3/A4/A8-recovery 既処理 file 再 touch 禁止境界
6. skip list 4 file untouched 継続

---

## §7 次 session 投入 prompt (fresh context 推奨)

```
AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-A-A5 着手前 trace を進めて。

読了必須 (6 件):
- docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A4-complete.md (本 handoff、A5 着手境界 source-of-truth)
- handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A3-complete.md
- handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A2-complete.md
- handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md
- handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A8-recovery-complete.md
- handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md

A5 scope: (F) extension sampler 残 (bumpMap/emissiveMap/altDiffuseMap/metallicRoughnessMap/occlusionMap 等) + pbrterrainF emissiveColors[4] terrain detail block

cadence 8 step (A5-trace → A5-prep → A5-patch [AYA 「OK」明示要] → A5-verify [self] → A5-handoff [shader cp + rm -rf cache + AYA launch verify] → A5-measurement → A5-commit [AYA 「OK commit して」明示要] → A5-complete handoff doc 起草)

hard rule 6 件: A4-complete §3.7 参照

feedback rule 12 件: doubt_self_first / no_scope_shrink / self_verify_before_handoff / use_agents_proactively / admit_unknown / falsification_as_progress / explanation_lead_with_conclusion / no_claude_coauthor / no_auto_commit / one_step_at_a_time / proactive_handoff / remove_verification_logs

1st action: A5-trace 着手前に AYA 「OK」明示確認 + 読了済 1 行 status 報告
```

---

## §8 cross reference

### §8.1 handoff doc 系譜

- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md` (A1 完遂、commit 99afb8f1bc)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A2-complete.md` (A2 完遂、commit 8e69841a53)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A8-recovery-complete.md` (A8-recovery 完遂、commit ac3f294643)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A3-complete.md` (A3 完遂、commit 524391d78d)
- **本 doc** (A4 完遂、commit 予定)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md` (bundle-A 全体 prep、commit 1434341904)
- `handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md` (β-2-hook 完遂、commit cecb9e6467)

### §8.2 patch commit 系譜

- `6f941c0a48` (A1 patch)
- `9f77f875db` (A2 patch)
- `aed1438936` (A8-recovery patch)
- `ebd5e2b16d` (A3 patch)
- **`f703710f8d` (A4 patch、本 handoff の直接 parent)**

### §8.3 spec sub-doc

- sub-doc 06 §1.2.2 / §1.2.4 / §3.1 sub-step 6.3
- sub-doc 07 §3.1 sub-step 7.2-7.4
- sub-doc 03 §3.1.3 (exemplar 2 役割)
- charter §3 #1 + §7.5

### §8.4 artifact

- binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (E) (本 A4 で補正確定) + §2 (F) (本 A4 + A5 source-of-truth)
- skip list `/tmp/skip-list-4.txt` (base 4 file: exemplar 2 + A2 拡張 2)
- project memory `project_ayastorm_r41_vulkan_migration.md` (γ'-port-β-2-bundle-A-A4 完遂 + γ'-port-β-2-bundle-A-A5 着手境界 active)

### §8.5 feedback rules 12 件

`doubt_self_first` (binding rule artifact §2 (E) 自己疑い → vec4[2]/vec3 補正) / `no_scope_shrink` / `self_verify_before_handoff` / `use_agents_proactively` (4 Agent 並列) / `admit_unknown` / `falsification_as_progress` / `explanation_lead_with_conclusion` / `no_claude_coauthor` / `no_auto_commit` (AYA 「OK commit して」明示指示下で commit) / `one_step_at_a_time` 遵守 / `proactive_handoff` / `remove_verification_logs`

---

**End of A4-complete handoff doc.** 次 action = fresh context で sub-step 4.3-γ'-port-β-2-bundle-A-A5 着手、本 doc §7 prompt を次 Claude session 投入用に使用可能。
