# Handoff — sub-step 4.3-γ'-port-β-2-bundle-A-A8-recovery 完遂

**起草日**: 2026-06-01
**commit**: `aed1438936`
**parent commits**: `9f77f875db` (A2) / `6f941c0a48` (A1) / `cecb9e6467` (β-2-hook) / `1434341904` (bundle-A-prep)
**ステータス**: A8-recovery 完遂 → 次 = A3 着手 (fresh context 推奨)

---

## §1 起草目的

A1/A2 で意図的に skip された AYAstorm 独自改造 11 file への遅延 UBO 注入が完遂したことを記録し、A3 (per-frame frame-global sampler binding qualifier 注入) 着手境界を fresh context へ引き継ぐ。

**認識すり合わせの履歴**:
- A1/A2 commit 時の skip 判断 = 「AYAstorm 独自改造 11 file は touch しない」(whole file untouched)
- AYA 確認時のすり合わせで判明: 本来意図は「既存の描画処理に合わせて処理を追加してきたつもり」= 3-段 swap pattern で GL path を `#else` で byte-for-byte 保全 + Vulkan path を `#ifdef LL_VULKAN_GLSL` で additive 注入
- AYA 判断 (2026-06-01) = (a) GL-path-preserved-via-3-段-swap が正、whole-file-untouched は誤読
- 対応 = A8-recovery sub-bundle 新設、本 commit で 11 file 中 8 file 遅延注入 (残 3 file は conditional injection 範式により除外)

---

## §2 A8-recovery 完遂 status

### §2.1 scope = 8 file 注入 (skip-list 11 file 中、conditional injection 範式により 3 file 除外)

**patched 8 file**:

| file | UBO 注入 | 行数差分 |
|---|---|---|
| Cinematic BD 2 | | |
| `cinematic_bd/class1/deferred/shadowUtil.glsl` | FrameLights | +31/-0 |
| `cinematic_bd/class3/deferred/screenSpaceReflUtil.glsl` | FrameViewProj | +14/-0 |
| Visual Realism 5 | | |
| `class1/deferred/blurLightF.glsl` | FrameViewProj | +17/-? |
| `class1/deferred/blurLightV.glsl` | FrameViewProj | +14/-0 |
| `class1/deferred/godraysF.glsl` | FrameViewProj + FrameLights + FrameAtmosphere | +53/-? |
| `class1/windlight/atmosphericsFuncs.glsl` | FrameLights + FrameAtmosphere | +44/-? |
| `class3/deferred/volumetricLightF.glsl` | FrameViewProj + FrameLights + FrameAtmosphere | +56/-? |
| Picker 1 | | |
| `class1/deferred/fsObjectIDV.glsl` | FrameViewProj | +14/-0 |

**合計**: 8 file / +226/-17 / 3 UBO block 個別 conditional injection

### §2.2 除外 3 file (conditional injection 範式)

`class1/deferred/fsObjectIDF.glsl` + `class1/deferred/godraysV.glsl` + `class1/deferred/volumetricLightF.glsl`:
- UBO member 参照 0 件 (FrameViewProj / FrameLights / FrameAtmosphere いずれも未参照)
- A1/A2 と同 conditional injection 範式により注入対象外
- 後段 A3 sampler binding 注入時に再評価対象 (sampler 参照あれば binding qualifier 注入)

### §2.3 重複処理事案と revert

**事案**: `class1/deferred/shadowUtil.glsl` (skip-list **外**、A1/A2 で既に注入済) を A8-recovery Agent が誤って consolidation refactor

**経緯**:
- A1 commit `6f941c0a48` で FrameViewProj 注入済
- A2 commit `9f77f875db` で FrameLights 注入済
- A8-recovery Agent が trace 段階の skip-list enforce 不徹底で対象に含めてしまい、fragmented form → consolidated form へ refactor (semantic 等価、機能変化 0、ただし scope 違反)

**対応**: AYA 「(a) で進めて」明示判断下、`git checkout -- class1/deferred/shadowUtil.glsl` で A1/A2 canonical form 復元、A8-recovery commit scope から除外

**教訓**: skip-list は「遅延注入対象 list」、A1/A2 で処理済 file は再 touch 禁止を絶対 rule として enforce (A3 着手時に Agent prompt に明示注入)

---

## §3 cold cache launch verify metric (2026-06-01)

| metric | A8-recovery | A2 baseline (`9f77f875db`) | A1 baseline (`6f941c0a48`) |
|---|---|---|---|
| hook fire | 224 | 224 | 224 |
| parse failed | 224/224 = 100% | 224/224 | 224/224 |
| location error | 152 | 152 | 151 |
| binding error | 37 | 37 | 37 |
| non-opaque uniforms outside block | 35 | 35 | 36 |
| missing #endif (cascade artifact) | 8 | 8 | 8 |
| GL compile/link fail | 0 | 0 | 0 |
| GL `.shaderbin` regenerate | 223 | 223 | 224 |
| FATAL/SIGSEGV/crash | 0 (10 件 mention は benign) | 0 | 0 |
| 起動成立 + clean shutdown | ✓ Goodbye!/Vulkan destroyed/status:stopped | ✓ | ✓ |

**観測**: A8-recovery metric が A2 baseline と同等 = glslang per-file first-error fail semantics により、A8 patch そのものは効いている (8 file 内の non-opaque uniform error 解消) が metric では大きな件数減少が見えない。bundle-A 全体完遂時の集合的閾値で評価 (A1/A2 と同 controlled measurement design)。

**charter §3 #1 acceptance**: GL path #else 内 byte-for-byte 維持 + 起動成立 + clean shutdown + GL regression 0 + crash 0 = 担保

---

## §4 A3 着手境界 (fresh context 推奨)

### §4.1 A3 scope

per-frame frame-global sampler binding qualifier 注入:

| sampler | binding |
|---|---|
| depthMap | set=0/binding=3 |
| lightMap | set=0/binding=4 |
| lightFunc | set=0/binding=5 |
| environmentMap (sampler2D + samplerCube 両対応) | set=0/binding=6 |
| reflectionProbes | set=0/binding=7 |
| shadowMap0-5 | set=0/binding=8-13 |
| noiseMap | set=0/binding=14 |
| cloud_noise_texture | set=0/binding=15 |
| cloud_noise_texture_next | set=0/binding=16 |
| glowNoiseMap | set=0/binding=17 |
| sceneMap | set=0/binding=18 |
| sceneDepth | set=0/binding=19 |
| brdfLut | set=0/binding=20 |
| exposureMap | set=0/binding=21 |

**設計差分 (A1/A2/A8 と A3)**:
- A1/A2/A8 = UBO block 化 (`layout(set=N, binding=M, std140) uniform <BlockName> { ... };`)
- A3 = individual sampler declaration (`layout(set=0, binding=M) uniform sampler2D <name>;` 直接注入、UBO block 不作)

**推定 file 数**: ~33 file 2 Agent 並列 (A8 で skip 解消した 5 file = cinematic_bd shadowUtil + cinematic_bd screenSpaceReflUtil + blurLightF + godraysF + volumetricLightF 込み)

### §4.2 environmentMap dual-type 注意

`environmentMap` は `sampler2D` 5 file + `samplerCube` 4 file の dual-type、同 binding set=0/binding=6 共有。Vulkan per-program 別 type 個別解決可能。file 毎 actual type grep 判定必須。

### §4.3 A3 着手前チェックリスト (10 件)

1. `git fetch` 実施 → HEAD `aed1438936` (A8-recovery commit) 確認
2. AYAstorm 改変 11 file リスト確定: 本 A8-recovery で 8 file UBO 注入済 + 3 file conditional 除外、A3 では sampler 参照判定で再評価
3. skip-list 11 file 体制 = **A8 で解消済** (sampler binding は同 file group でも再注入可能、ただし重複処理防止のため A3 Agent prompt に「A1/A2/A8 で既処理 file の UBO block は再 touch 禁止」を明示注入)
4. binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (D) 再読み込み (/tmp persist 性 fragile = 失効時は本 doc §4.1 表または bundle-A-prep `1434341904` §2 (D) から再生成)
5. bundle-A-prep `1434341904` 読了
6. β-2-hook-complete `cecb9e6467` 読了
7. A1-complete handoff 読了
8. A2-complete handoff 読了
9. **本 A8-recovery-complete handoff 読了** (本 doc)
10. project memory `project_ayastorm_r41_vulkan_migration.md` 確認 + AYA 「OK」承認待ち

### §4.4 推奨 cadence (8 step、A1/A2/A8 範式継承)

1. **A3-trace**: Claude grep + union + environmentMap type 判定 + Agent prompt 分割
2. **A3-prep**: 2 Agent prompt 構築
3. **A3-patch**: Agent 並列起動 (AYA 「OK」明示指示要)
4. **A3-verify**: Claude self-verify (sampler binding canonical + GL #else 保全 + UBO block 既処理 untouched)
5. **A3-handoff**: cache 完全 clear (`rm -rf ~/.ayastorm_x64/cache/`) + shader cp + AYA launch + clean shutdown 確認依頼
6. **A3-measurement**: Claude log 解析 (期待 binding error 37 → 大幅減少観測可能性あり、sampler binding 単独で完結する file 複数あり)
7. **A3-commit**: AYA 「OK commit して」明示指示下
8. **A3-complete**: handoff doc 起草

---

## §5 範式 (A1/A2/A8 共通、A3 へ継承)

### §5.1 hard rule 5 件

1. UBO declaration byte-for-byte canonical (A1=FrameViewProj 9 member / A2=FrameLights 9 member + FrameAtmosphere 18 member / A8 同 canonical literal 使用)
2. 3-段 swap pattern 厳守 (`#ifdef LL_VULKAN_GLSL` <Vulkan> `#else` <GL byte-for-byte> `#endif`)
3. binding 番号 §4.1 表遵守 (A3 sampler) / bundle-A-prep §2 (A)-(D) 表遵守
4. 1 file 1 patch 横断 share 禁止
5. **A1/A2/A8 で処理済 file の UBO block は再 touch 禁止** (A3 では sampler binding qualifier のみ追加注入)

### §5.2 conditional injection 範式 (A8 継承)

file が当該 UBO/sampler member を **1 件でも参照する場合のみ注入**。0 件なら skip (file 機能不変)。A3 では sampler 毎に file の参照判定が必要。

### §5.3 GL path #else 保全

`#else` 分岐内に既存 GL uniform/sampler 列を byte-for-byte 維持。コメント位置移動は許容 (godraysF 範式継承、両分岐共通コメント化)。

### §5.4 isolated `#ifndef LL_VULKAN_GLSL` wrap

primary group 外散在 member 宣言は `#ifndef LL_VULKAN_GLSL ... #endif` で条件付き skip (body code 不変担保)。A1=42 / A2=14 / A8=0 occurrence。A3 では sampler 重複宣言があれば同範式適用。

---

## §6 AYA 承認境界 (A3 進行時に確認要)

1. 本 A8-recovery commit `aed1438936` の review
2. A3 着手指示 (明示「OK」必要)
3. A3-patch Agent 並列起動
4. A3 commit
5. **既処理 file 再 touch 禁止 (A3 sampler binding のみ追加、UBO block 不変)**
6. 11 file → 0 file の skip-list 解消の継続 (本 A8 で完了済、A3 以降 skip 概念は exemplar 2 + A2 拡張 2 のみ)

---

## §7 残 skip 既知 list (A8 後の状況)

- **exemplar 2** (`class1/deferred/diffuseV.glsl` + `class1/deferred/diffuseF.glsl`) = β-1 PoC 試作レール継続、untouched (sub-doc 03 §3.1.3 役割)
- **A2 拡張 skip 2** (`previewV.glsl` + `multiPointLightF.glsl`) = type divergence / per-pass local light で個別対応必要、後段 sub-bundle で再設計
- **A8-recovery 後の AYAstorm 独自改造 11 file** = **全 file UBO 注入完了** (8 file patched + 3 file conditional 除外)、A3 sampler binding 注入時に通常 file と同等扱い

---

## §8 risks/caveats (10 件)

### §8.1 measurement plan 楽観性再是正
A2 段階で glslang per-file first-error fail semantics 確認、A3 で binding error 37 件大幅減少観測可能性あり (sampler binding は location/uniform より下位 error)、bundle-A 全体集合的閾値で評価。

### §8.2 cold cache launch verify 必須
A3 verify 前 `rm -rf ~/.ayastorm_x64/cache/` 必須 (β-2-hook bypass 防止)。A1/A2/A8 で確認済範式。

### §8.3 glslang missing #endif cascade artifact
A1=8 / A2=8 / A8=8 件変化なし。bundle-A 全体完遂で自然消滅。A3 段階でも変化なし期待。

### §8.4 conditional injection 範式 A3 継承
sampler 個別宣言で file が sampler 1 件でも参照する場合のみ binding qualifier 注入。

### §8.5 既処理 file 再 touch 禁止 (A8 反省事項)
A3 では A1/A2/A8 で既処理の UBO block を絶対に touch しない (重複処理 = scope 違反 + canonical form 改変 risk)。

### §8.6 environmentMap dual-type
`sampler2D` vs `samplerCube` 同 binding set=0/binding=6 共有可、per-program 個別解決。file 毎 actual type grep 判定必須。

### §8.7 binding rule artifact persist 性
`/tmp/bundle-A-binding-rules.md` 失効時は bundle-A-prep `1434341904` §2 (D) 表または本 doc §4.1 から再生成。

### §8.8 AYAstorm 独自改造 untouched 維持解除
本 A8 で skip-list 11 file 体制は **解消** (全 file 注入完了 or conditional 除外)。A3 以降は exemplar 2 + A2 拡張 2 のみ skip 対象。

### §8.9 exemplar 2 (diffuseV/F) untouched 維持
β-1 PoC 試作レール継続。A3 でも untouched (sub-doc 03 §3.1.3 役割再定義)。

### §8.10 context budget concern
A3 session 内で trace + prep + 2 Agent 並列 patch + self-verify + AYA handoff + measurement + commit + handoff doc の 8 step 全部、fresh context 推奨 (A1/A2/A8 で実証済)。

---

## §9 次 session 投入 prompt (fresh context 推奨)

**読了必須 (8 件)**:
1. 本 A8-recovery-complete handoff doc (本 doc)
2. A2-complete handoff `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A2-complete.md`
3. A1-complete handoff `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md`
4. bundle-A-prep `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md` (commit `1434341904`)
5. β-2-hook-complete `handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md` (commit `cecb9e6467`)
6. binding rule artifact `/tmp/bundle-A-binding-rules.md` §2 (D) (失効時は §4.1 から再生成)
7. project memory `project_ayastorm_r41_vulkan_migration.md`
8. sub-doc 06 §1.2.2/§1.2.4/§3.1 sub-step 6.3 + sub-doc 07 §3.1 sub-step 7.2-7.4 + charter §3 #1 + §7.5

**A3 scope**: §4.1 表 (19 sampler 個別 binding qualifier 注入、推定 ~33 file 2 Agent 並列)

**cadence 8 step**: §4.4

**hard rule 5 件**: §5.1

**feedback rule (10 件)**:
- `feedback_doubt_self_first` (skip-list 解釈の自己疑い継承)
- `feedback_no_scope_shrink` (literal scope 厳守)
- `feedback_self_verify_before_handoff`
- `feedback_use_agents_proactively`
- `feedback_admit_unknown`
- `feedback_falsification_as_progress`
- `feedback_explanation_lead_with_conclusion`
- `feedback_no_claude_coauthor`
- `feedback_no_auto_commit` (AYA 明示指示下のみ)
- `feedback_one_step_at_a_time`
- `feedback_proactive_handoff`
- `feedback_remove_verification_logs`

**1st action**: A3-trace 着手前 AYA 「OK」確認 + 読了済 1 行 status 報告

---

## §10 cross reference

**handoff doc 4 件**:
- A1-complete (本 commit `aed1438936` の grandparent 範式)
- A2-complete (本 commit の parent 範式)
- bundle-A-prep (`1434341904`)
- β-2-hook-complete (`cecb9e6467`)

**spec sub-doc**:
- sub-doc 06 §1.2.2/§1.2.4/§3.1 sub-step 6.3
- sub-doc 07 §3.1 sub-step 7.2-7.4
- charter §3 #1 + §7.5
- sub-doc 03 §3.1.3 (exemplar 2 役割)

**commit reference**:
- A1 patch `6f941c0a48`
- A2 patch `9f77f875db`
- A8-recovery patch `aed1438936` (本)

**memory**:
- `project_ayastorm_r41_vulkan_migration.md` (γ'-port-β-2-bundle-A-A8-recovery 完遂 + γ'-port-β-2-bundle-A-A3 着手境界 active)

**artifact**:
- repo 内: (A8-recovery で skip-list 概念は解消、bundle-A-skip-list.txt は exemplar 2 + A2 拡張 2 のみ保持)
- /tmp 配置: `bundle-A-binding-rules.md` (失効性あり、§2 (D) は本 doc §4.1 source-of-truth 優先)
