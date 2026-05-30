# AYAstorm r41 sub-doc 06-shader-spirv — 領域 6 248 GLSL shader SPIR-V 化 base port

**status**: **closed 2026-05-29 (Pattern α 一括 draft、AYA review PASS)**
**親 charter**: `docs/specs/ayastorm-r41-gl-removal/00-charter.md` (closed 2026-05-28)
**並走 sub-doc**: `03-state-machine-pso.md` (段階 3 = 領域 3、PSO 配線で SPIR-V binary 必要) + `07-descriptor-renderpass.md` (領域 7 = descriptor set binding と shader binding の整合)
**前 handoff**: `handoff-stage-2-complete.md` (段階 2 完遂 → 段階 3 着手境界 + 領域 6/7 並走起草判断 B 採用 2026-05-28)
**達成条件**: 領域 6 完遂 = 248 GLSL shader のうち **base 228 file** (cross compile A 195 素通り + B 53 要修正) を `glslang` で SPIR-V 化 + runtime `vkCreateShaderModule` 経由 load + descriptor set binding mismatch validation 0 件。AYAstorm 改変 13 file は **r42-α/β/γ scope 外、本領域では untouched 維持**
**関連 charter section**: §2 領域 6 (4.96 PM、単一最大領域) + §3 #4 acceptance (228 file SPIR-V 化 + AYAstorm 13 file untouched) + §7.4 sub-doc 構成 + §7.5 boundary

---

## §1 領域 6 scope plan

### §1.1 領域 6 scope 再掲 (charter §2 領域 6 + 04 §1.3 + 05 §2)

- **対象**: 248 GLSL shader 全 file のうち **base 228 file** (cross compile A 195 素通り + B 53 要修正)
- **境界条件 (charter §2 領域 6)**: 領域 3 PSO 化 + 領域 7 descriptor set 整合と協調 (shader binding が descriptor set 3 階層 set=0/1/2 に従う)
- **依存順序 (charter §2 領域 6)**: 領域 1 (段階 1) 完了後着手、領域 2 (段階 2 完遂済) + 領域 3 (段階 3) + 領域 7 と並走必須
- **risk 性質 (charter §2 領域 6)**: **中** — glslang cross compile ~85% 素通り見込み (05 §2.2 確定値)、B 53 file の修正は内容次第 (extension / binding / precision / `attribute|varying` legacy 残存)、06 §3.1 で base 算定済
- **AYAstorm 改変 13 file の取扱**: **r42-α (picker 2 file) / r42-β (Cinematic 4 file) / r42-γ (visual realism 7 file) scope 外**、本領域 6 では untouched 維持 (charter §3 #4 acceptance regression 担保)

### §1.2 shader file 分類 + 実装方針 (04 §1.3 + 05 §2.3 反映)

#### §1.2.1 base 248 file (本領域 6 対象)

a-4 §1.3 directory 別:

| directory | file 数 | 性質 | 領域 6 実装方針 |
|---|---|---|---|
| class1/deferred | 120 | deferred rendering core (g-buffer / soften lighting / sky / atmospherics) | glslang `--target-env vulkan1.3` 直 compile、descriptor set binding は 07 §3 set=0/1/2 と整合、AYAstorm 改変済 file (picker / Cinematic / visual realism = 13 file) は **本領域で touch しない** |
| class1/interface | 44 | UI (2D / font / cursor) | descriptor set=0 per-frame 中心 (texture atlas + 2D matrix UBO)、push constant で widget position |
| class3/deferred | 16 | high-end feature (SSAO / DoF / reflection probe 等) | A 素通り見込み高い (a-4 §1.3 + 05 §2.2)、ただし AYAstorm 改変 7 file (visual realism r42-γ scope) は本領域で touch しない |
| class1/objects | 14 | object 描画 (avatar / mesh / terrain) | terrain shader は段階 3 sub-step 3.4 (terrain glTexGen 廃止) と並走、shader 側 explicit UV attribute 化を本領域で配信 (terrain shader port は領域 3 + 領域 6 ジョイントで satisfy) |
| その他 | 54 | windlight / cinematic_bd / 等 | windlight base shader は本領域 port、cinematic_bd の AYAstorm 改変 4 file (r42-β scope) は本領域で touch しない |

#### §1.2.2 cross compile 分類 (05 §2.2 反映)

| cross compile classification | file 数 | 内容 | 領域 6 実装方針 |
|---|---|---|---|
| **A: 素通り** | 195 file | glslang `--target-env vulkan1.3` で 1 pass compile 成功、修正不要 | sub-step 6.2 で一括 build script 通して `.spv` 生成成功 verify |
| **B: 要修正** | 53 file | 残 15% issue (built-in 座標 `gl_FragCoord` / `gl_PointCoord` Y 反転、`attribute|varying` legacy 残存、`#extension GL_ARB_*` 依存、`precision highp|mediump|lowp` qualifier) | sub-step 6.3 で issue type 別 bundle で修正、各修正後再 cross compile verify |
| **AYAstorm 改変 13 file** | 13 file | r42-α/β/γ scope 外 | sub-step 6.5 で untouched 維持 verify (`git diff` で 13 file が touch 0 件確認) |

### §1.3 並走領域との関係 (charter §2 領域 6 依存順序)

| 並走領域 | 領域 6 内での協調事項 |
|---|---|
| 領域 3 (段階 3 state machine → PSO 化) | PSO compile 時に SPIR-V binary 必要、shader binding (vertex attribute / push constant / descriptor set) が段階 3 PSO layout と一致する必要、段階 3 sub-step 3.3 matrix stack → push constant 化と shader 側 push constant 受領が同期。**段階 3 sub-step 3.3-B (1 shader exemplar pre-flight、sub-doc 03 §3.1.3 / 2026-05-30 着手境界) は本領域 6 sub-step 6.1 (autobuild integration 一括化) の pre-flight として位置付け** — 3.3-B で確立する CMake target `aya_shader_compile` + glslang build 時 pre-compile chain + `vkCreateShaderModule` load + 二段構え matrix shader 側受領 pattern を本領域 sub-step 6.1 で 248 file 全体へ一般化、本領域 sub-step 6.1 の design ground 確定済 |
| 領域 7 (descriptor set + render pass) | shader binding (set=0/1/2 + binding 番号) と 07 §3 descriptor set 3 階層が一致、shader 側 `layout(set=N, binding=M)` qualifier 配信を領域 7 の `VkDescriptorSetLayoutBinding` 設計と同期 |
| 領域 2 (lldrawpool、段階 2 完遂済) | 12 pool hook body PSO bind 時に shader binding 整合 (段階 3 sub-step 3.4 と並走)、terrain pool は terrain shader explicit UV 化と同期 |
| 領域 8 (LLVKRenderer skeleton) | shader load API (`vkCreateShaderModule` 経由) の signature が r41.5 interface 経由 call 化前提に整合 (charter §3 #6 担保) |

---

## §2 cross compile 順序 + dependency graph

### §2.1 cross compile 順序の決定軸 3 点

1. **A 素通り 195 file 先行** — cross compile 素通り想定の 195 file は機械的 build script 通しで `.spv` 生成、acceptance #4 base portion の 80% を早期 satisfy + B 53 file の typical issue 抽出母数として A 通過 baseline 確立
2. **B 53 file の issue type 別 bundle 修正** — built-in 座標 Y 反転 / legacy `attribute|varying` / `#extension GL_ARB_*` / `precision` qualifier の 4 issue type に分類、type 別 bundle で修正 (Agent 並列活用候補)
3. **descriptor set binding 統合は領域 7 同期** — shader 側 `layout(set=N, binding=M)` qualifier 配信は領域 7 の `VkDescriptorSetLayoutBinding` 設計が確定してから (07 §3 sub-step 7.2-7.4 と sub-step 6.4 が同期)

### §2.2 dependency graph (4 階層、A 素通り → B 修正 → descriptor binding 統合 → untouched verify)

```
glslang + SPIRV-Cross + autobuild integration (sub-step 6.1)
├── A 195 file 素通り cross compile (sub-step 6.2、acceptance #4 base portion 80% 早期 satisfy)
│   └── class1/deferred 大半 + class1/interface 大半 + class3/deferred 一部 + class1/objects 大半 + その他大半
├── B 53 file 要修正 (sub-step 6.3、issue type 別 bundle)
│   ├── built-in 座標 Y 反転 (glslang --invert-y で吸収可な file)
│   ├── legacy `attribute|varying` 残存 (`in|out` 書換)
│   ├── `#extension GL_ARB_*` 依存 (SPIR-V capability mapping audit)
│   └── `precision highp|mediump|lowp` qualifier (Vulkan portability 要、Mac MoltenVK 整合)
├── descriptor set binding 統合 (sub-step 6.4、領域 7 §3 sub-step 7.2-7.4 と同期)
│   └── shader 側 `layout(set=N, binding=M)` qualifier 配信 + matrix stack → push constant 受領
└── 領域 6 self-check + AYAstorm 13 file untouched verify (sub-step 6.5)
    └── `git diff` で 13 file が touch 0 件確認 + acceptance #4 全 metric PASS
```

### §2.3 並列着手可能性

- A 195 file の cross compile 素通り検証は file 単位で independent → Agent 並列活用 (`feedback_use_agents_proactively.md` 反映)、build script で一括処理が efficient
- B 53 file の issue type 4 bundle は bundle 間で independent → 並列着手可能 (issue type 別に Agent 分担)
- descriptor binding 統合 (sub-step 6.4) は領域 7 の sub-step 7.2-7.4 と同期必須 → 並走 cadence は領域 7 sub-doc draft 進捗 base で AYA + Claude 擦り合わせ
- AYAstorm 13 file untouched verify (sub-step 6.5) は最終 self-check phase で実施、本 sub-doc では sub-step 6.5 の単一 phase に集約

---

## §3 領域 6 sub-step 順序 (5 sub-step 化、段階 2/3 範式継承)

`02-portage-execution.md` §3 + `03-state-machine-pso.md` §3 範式継承 (5 sub-step + 末尾 self-check)、§2.2 dependency graph に沿って bundle。

### §3.1 sub-step list

| sub-step | scope | 対象 (file 数) | 完了 marker |
|---|---|---|---|
| **6.1** | glslang + SPIRV-Cross + autobuild integration (**段階 3 sub-step 3.3-B pre-flight で確立済 pattern を 248 file 一般化**) | LunarG SDK 1.3.x 同梱 glslang + SPIRV-Cross binary + cmake target `aya_shader_compile` (3.3-B で新規追加済) を 248 file scope へ拡張 + autobuild package 配置 + `indra/newview/app_settings/shaders/aya_r41_exemplar/` (3.3-B 配置) を class1/deferred/ 等正規 path へ移管判断 | `cmake` configure pass + `aya_shader_compile` target build pass + 任意 1 file (例: `class1/deferred/diffuseV.glsl`) → `.spv` 生成成功 + `vkCreateShaderModule` load 成功 (validation 0 件)、3.3-B exemplar pre-flight 完遂前提 |
| **6.2** | A 195 file 素通り cross compile | A 分類 195 file (class1/deferred 大半 + class1/interface 大半 + class3/deferred 一部 + class1/objects 大半 + その他大半) | 195 file 全 `.spv` 生成成功 + runtime `vkCreateShaderModule` load 成功 + validation 0 件、acceptance #4 base portion 80% 早期 satisfy |
| **6.3** | B 53 file 要修正 (issue type 別 bundle) | B 分類 53 file を 4 issue type bundle 化 (built-in 座標 Y 反転 / legacy `attribute|varying` / `#extension GL_ARB_*` / `precision` qualifier) | 53 file 全 `.spv` 生成成功 + runtime load 成功 + validation 0 件、issue type 別修正内容 doc 化 (handoff 含み) |
| **6.4** | descriptor set binding 統合 (領域 7 同期) | shader 側 `layout(set=N, binding=M)` qualifier 配信 (set=0 per-frame / set=1 per-material / set=2 per-draw) + matrix stack → push constant 受領 (段階 3 sub-step 3.3 同期) | 228 file 全 shader binding が 07 §3 descriptor set 3 階層と一致 + validation layer で descriptor binding mismatch 0 件 + 段階 3 PSO compile 成功 |
| **6.5** | 領域 6 self-check + AYAstorm 13 file untouched verify + handoff doc | (本 sub-step) | §4.1 acceptance 4 件 self-trace PASS (charter §3 #4 領域 6 分 + AYAstorm 13 file untouched + regression) + `git diff` で 13 file touch 0 件確認 + handoff doc `handoff-stage-6-complete.md` 作成 |

### §3.2 sub-step 内 file 順序の柔軟性 (charter §7.5 boundary refine 可)

- sub-step 6.2 の A 195 file は順序問わず一括 build script 処理 → Agent 並列活用が efficient
- sub-step 6.3 の B 53 file は issue type 4 bundle (各 ~10-15 file) → bundle 間並列着手可能、Agent 分担
- sub-step 6.4 descriptor binding 統合は領域 7 sub-step 7.2-7.4 と同期必須 → 領域 7 sub-doc draft 進捗 base で着手 timing 調整

### §3.3 領域 6 で touch しない file (本領域 scope 外)

- **AYAstorm 改変 13 file** (r42-α picker 2 / r42-β Cinematic 4 / r42-γ visual realism 7) — 本領域で touch 0 件 (sub-step 6.5 で `git diff` verify)
- llrender 主要 5 file (段階 3 scope)、lldrawpool 13 file C++ 側 (段階 2 完遂済 + 段階 3 sub-step 3.4 で hook body PSO bind 配線)
- pipeline.cpp 3 大グローバル (段階 4 scope)、llspatialpartition / llviewershadermgr / llvertexbuffer / llvosky / llvowlsky (段階 5 scope)
- descriptor set layout / pipeline layout / render pass chain 実装 (領域 7 scope、本領域は shader 側 `layout()` qualifier 配信のみ)
- LLVKRenderer pipeline.cpp inline 実装 (段階 4 + 領域 8 scope)

---

## §4 領域 6 completion criteria

charter §3 acceptance criterion #4 (228 file SPIR-V 化 + AYAstorm 13 file untouched) + #3 (段階 1-5 全完遂と協調) の **領域 6 分 self-check**。

### §4.1 領域 6 自己 acceptance

| criterion | metric | test procedure |
|---|---|---|
| **#4-領域 6 (A 195 file 素通り)** | A 分類 195 file 全 `.spv` 生成成功 + runtime `vkCreateShaderModule` load 成功 + validation 0 件 | `aya_shader_compile` build 成功 (`*.spv` 195 file 全件生成) + viewer 起動時 LL_INFOS log で 195 file shader load 成功 + validation layer で `vkCreateShaderModule` error 0 件 |
| **#4-領域 6 (B 53 file 修正後素通り)** | B 分類 53 file の issue type 別修正後 `.spv` 生成成功 + runtime load 成功 + validation 0 件 | sub-step 6.3 完遂後 build 成功 + viewer 起動時 53 file 全 load 成功 + validation 0 件 + 修正内容 (issue type 4 bundle) handoff doc 反映 |
| **#4-領域 6 (descriptor set binding 整合)** | 228 file 全 shader 側 `layout(set=N, binding=M)` qualifier が 07 §3 descriptor set 3 階層 (set=0 per-frame / set=1 per-material / set=2 per-draw) と一致 + 段階 3 PSO compile 成功 | `grep -rE "layout\(set=" indra/newview/app_settings/shaders/` で 228 file 全 file 内 set= qualifier hit + validation layer で descriptor binding mismatch / push constant range mismatch **0 件** + 段階 3 sub-step 3.4 PSO compile success log 確認 |
| **#4-領域 6 (AYAstorm 改変 13 file untouched)** | 領域 6 全 sub-step で 13 file (picker 2 / Cinematic 4 / visual realism 7) が **touched 0 件** | `git diff feature/ayastorm-r41-gl-removal..HEAD -- <13 file path 列挙>` が **0 件 hit** (charter §3 #4 acceptance regression 担保) |
| **regression (段階 1-3 動作維持)** | 段階 1-3 acceptance 維持 + viewer 起動 + AYAstorm 機能 (audio / chat / login / inventory) regression 0 件 | `01-foundation.md` §4.1 + `02-portage-execution.md` §4.1 + `03-state-machine-pso.md` §4.1 acceptance 再 verify + sustained ~10 分動作 + AYA 起動確認 PASS (`feedback_release_with_user_feedback.md` 遵守) |

### §4.2 不達時の対処 (charter §3 acceptance 運用方針継承)

- 5 criterion のうち 1 件でも未達 = 領域 6 未達 (段階 4 + 領域 7 着手保留判断、charter §7.5 boundary refine で領域別 partial pass も可)
- 未達 criterion 別に対処 (例: #4-領域 6 で specific B file の `precision` qualifier mismatch validation error 残存 → glslang `--target-spv 1.5` 等 target version refine → 再 cross compile → verify)
- **defer / disable 提案 ban** (`feedback_self_bug_no_defer_option.md` 遵守、fix 案のみ提示)
- **仮説 2 連続外れ rule** (`feedback_admit_unknown.md` 遵守): cross compile error / SPIR-V binary mismatch / descriptor binding violation で仮説 2 連続外れたら `glslangValidator -V -S <stage> --target-env vulkan1.3 -o <out>.spv <in>.glsl` 個別実行 + SPIRV-Cross reflect 出力 / spv-val validation で実データ取得に切替

### §4.3 領域 6 完遂後の次 段階

- **段階 4 着手判断** (pipeline.cpp 3 大グローバル → LLPipelineFrameContext、charter §2 領域 4、本領域 6 完遂で領域 3 完了済前提)
- **領域 7 完遂判断**: 本領域 6 と並走の領域 7 sub-doc 07 が completion 状態か AYA 確認
- **handoff doc**: `handoff-stage-6-complete.md` 作成 (領域 6 完遂境界、`feedback_proactive_handoff.md` 遵守)
- **AYAstorm 改変 13 file r42 移行 prep**: r41 完遂後 r42-α/β/γ で 13 file SPIR-V port、本領域 6 で確立した cross compile chain + descriptor binding pattern を r42 で流用

---

## §5 関連 doc / memory

### §5.1 直接参照 doc

| doc | 本 sub-doc での参照 section |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | §2 領域 6 (4.96 PM 単一最大領域 中 risk) + §3 #4 acceptance + §7.4 sub-doc 構成 + §7.5 boundary refine 可 |
| `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` | §3.4 sub-step 範式継承 + §3.5 段階 1 で touch しない file の領域 6 並走着手前提反映 |
| `docs/specs/ayastorm-r41-gl-removal/02-portage-execution.md` | §3.3 段階 2 で touch しない file の領域 6 並走前提 + §1.3 並走領域協調事項 (terrain shader 配信は段階 3 と本領域でジョイント satisfy) |
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` (本 sub-doc と同時起草) | §1.3 並走領域協調事項 + §3 sub-step 3.3 matrix stack → push constant 化と本領域 sub-step 6.4 同期 + §3 sub-step 3.4 terrain shader 配信と本領域 sub-step 6.3 同期 + **§3.1.3 sub-step 3.3-B (1 shader exemplar pre-flight、2026-05-30 着手) と本領域 sub-step 6.1 (autobuild integration 一括化) の役割分担** (3.3-B = exemplar / 6.1 = 一般化、3.3-B で確立した CMake target + glslang build chain + `vkCreateShaderModule` load + 二段構え matrix shader 受領 pattern を 6.1 で 248 file 全体へ拡張) |
| `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` (本 sub-doc と同時起草) | §3 descriptor set 3 階層 + §3 sub-step 7.2-7.4 と本領域 sub-step 6.4 同期 (shader 側 `layout(set=N, binding=M)` qualifier 配信が領域 7 の `VkDescriptorSetLayoutBinding` 設計に従う) |
| `docs/specs/ayastorm-r40-vulkan-migration/04-portage-inventory.md` | §1.3 GLSL shader 248 file 内訳 (class1/2/3 deferred + interface + lighting + windlight + cinematic_bd) + §5.1-§5.3 AYAstorm 機能 shader touchpoint 13 file (r42 scope 外明示) |
| `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` | §1.3 LunarG SDK 1.3.x + glslang + SPIRV-Cross + VMA 採用 (本領域 sub-step 6.1 integration) + §2 shader cross compile chain (本 §1.2 + §2 base) + §2.2 GLSL feature → SPIR-V capability mapping (compute/geometry/tessellation 全 0、~85% 素通り見込) + §2.3 base 248 + AYAstorm 改変 13 取扱 + §2.4 GLSL extension audit 要点 + §2.5 build integration |
| `docs/specs/ayastorm-r40-vulkan-migration/06-effort-estimation.md` | §3.1 + §3.2 領域 6 PM 4.96 (本 §1 + §3 領域 6 scope 整合) |

### §5.2 関連 memory

| memory | 本 sub-doc での参照 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | r41 milestone active 状態 + 領域 6 並走着手前提 |
| `project_ayastorm_three_platforms.md` | 3 OS 大前提 + Linux 先行例外 (本領域 6 も Linux 限定動作確認、§4.1 #regression 反映)、Mac MoltenVK は SPIR-V → MSL 内部変換 (sub-step 6.4 portability subset 整合) |
| `reference_deferred_shader_routing.md` | sub-step 6.4 shader binding 整合時の deferred shader routing reference |
| `project_aya_visual_realism_alpha_protect.md` | AYAstorm 改変 7 file (visual realism、r42-γ scope 外) の `frag_color.a = 0` invariant 認識 (本領域 untouched 維持で保護) |

### §5.3 関連 feedback

| feedback | 本 sub-doc での参照 |
|---|---|
| `feedback_experiment_branch_single_scope.md` | branch scope 単一性 (r41 内 sub-branch 作らない、`01-foundation.md` §2.3 継承) |
| `feedback_no_auto_commit.md` | commit は AYA 指示後 |
| `feedback_release_flow.md` | push は AYA 手動 |
| `feedback_self_bug_no_defer_option.md` | §4.2 defer / disable 提案 ban + AYAstorm 13 file untouched 維持の選択肢化禁止 |
| `feedback_self_verify_before_handoff.md` | §3 sub-step 6.5 self-check + §4.3 領域 6 完遂境界 self-trace |
| `feedback_proactive_handoff.md` | §4.3 領域 6 完遂時の handoff doc 作成 |
| `feedback_build_only_verified.md` | §4 completion criteria の satisfy は実機検証 (推論 ban) |
| `feedback_admit_unknown.md` | §4.2 cross compile / SPIR-V binary mismatch / descriptor binding violation で仮説 2 連続外れたら glslangValidator 個別実行 + SPIRV-Cross reflect + spv-val 切替 |
| `feedback_use_agents_proactively.md` | §2.3 + §3.2 sub-step 6.2 A 195 file 一括 build script + sub-step 6.3 B 53 file issue type 4 bundle で Agent 並列活用 |
| `feedback_perf_map_bfs_drill.md` | §2.2 dependency graph の階層的 sub-step 化 (integration → A 素通り → B 修正 → descriptor binding → untouched verify) |
| `feedback_release_with_user_feedback.md` | §4.1 #regression の exhaustive solo session 不要 (AYA 起動確認 PASS で sufficient) |
| `feedback_one_step_at_a_time.md` | §3 sub-step 単位で 1 メッセージ 1 アクション着手 |

---

## §6 起草 cadence + 完成宣言

### §6.1 本 sub-doc 起草情報

- **起草着手**: 2026-05-28
- **起草主体**: AYA + Claude
- **起草先**: `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` (本 doc)
- **起草 cadence**: **Pattern α (一括 draft)** — `01-foundation.md` / `02-portage-execution.md` / `03-state-machine-pso.md` 範式継承、sub-doc は内容具体 (228 file 分類 / 5 sub-step / acceptance 5 件) で section 数少なく、Pattern β 分割 overhead 回避 (charter §7.5 boundary refine 可)
- **scope**: **領域 6 only + AYAstorm 13 file untouched 維持** (228 file SPIR-V 化 base port)、r42-α/β/γ の AYAstorm 改変 13 file port は別 milestone scope
- **並走起草 sub-doc**: 同 session で `03-state-machine-pso.md` (段階 3 = 領域 3) + `07-descriptor-renderpass.md` (領域 7) を Pattern α 一括起草、3 doc 同時 AYA review (handoff-stage-2-complete §2.3 B 案、AYA 採用)

### §6.2 sub-doc 番号付与の justification (charter §7.4 outline → 領域番号同期 refine)

`03-state-machine-pso.md` §6.2 と同等の justification。本 sub-doc は **`06-shader-spirv.md`** で確定 (charter §7.4 outline `03-shader-port.md` から refine、領域番号 = sub-doc 番号同期方針)。

| charter §7.4 outline | 本 r41 章実採用 | 採用理由 |
|---|---|---|
| `03-shader-port.md` | **`06-shader-spirv.md`** | sub-doc 番号 = charter §2 領域番号 (領域 6) で統一、r41 章内 cross reference 整合 |

本 §6.2 の番号 refine は charter §7.5 boundary refine 可の範囲内、AYA review で承認後に正式採用。

### §6.3 完成宣言条件

本 sub-doc は **AYA review PASS で完成宣言**、status field を `closed YYYY-MM-DD (Pattern α 一括 draft + AYA review PASS、領域 6 着手準備 ready)` に更新。

完成宣言後の次 action:

- 領域 6 sub-step 6.1 着手 (glslang + SPIRV-Cross + autobuild integration)
- 領域 6 sub-step 6.5 完遂時に handoff doc `handoff-stage-6-complete.md` 作成
- r41 全完遂後の r42-α/β/γ で AYAstorm 改変 13 file SPIR-V port (本領域 6 で確立した cross compile chain + descriptor binding pattern を流用)

### §6.4 本 sub-doc commit 反映

本 sub-doc 完成宣言 commit は AYA 明示指示後 Claude が実施。commit message draft (AYA 指示時 refine 可):

```
docs(r41): sub-doc 06-shader-spirv.md 完成 + 領域 6 prep

- 06-shader-spirv.md 新規作成 (Pattern α 一括 draft + AYA review PASS)
- §1 領域 6 scope (228 file SPIR-V 化、4.96 PM 単一最大領域 中 risk、領域 3/7 並走必須)
- §1.2 A 195 素通り + B 53 要修正 + AYAstorm 13 untouched 分類
- §2 cross compile 順序 + dependency graph (integration → A → B → descriptor binding → untouched verify 4 階層)
- §3 領域 6 sub-step 5 件 (6.1 integration / 6.2 A 195 / 6.3 B 53 / 6.4 descriptor binding / 6.5 self-check + untouched verify)
- §4 領域 6 completion criteria 5 件 (A 素通り / B 修正 / descriptor 整合 / AYAstorm untouched / regression)
- §5 関連 doc/memory + §6 起草 cadence (Pattern α / 領域 6 only + AYAstorm 13 untouched)
- §6.2 sub-doc 番号付与 refine justification (charter §7.4 outline → 領域番号同期)
```

push は AYA 手動 (`feedback_release_flow.md` 遵守)。
