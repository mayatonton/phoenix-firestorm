# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-α cold launch verify prep handoff

**作成日**: 2026-06-03
**branch**: `feature/ayastorm-r41-gl-removal`
**前 handoff (= 設計 pivot)**: `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-pivot-to-ubo-design.md`
**設計 chapter 群 (= source of truth)**: `design/01-10` + `06a/06a-prep/06b/06c`
**audit 集約 doc**: `audit-past-b-work-vs-design-2026-06-03.md`
**第二次査読 report**: `design-review-2026-06-03-second-pass.md`

---

## §0 本 handoff の目的 (= 次 session 着手地点)

η-28 設計 pivot 後、設計 chapter 群 (01-10) 起案完了 + 第二次査読 (PASS-with-major-fixes) + 過去 B 作業整合性 audit (整合率 100% / blueprint 不在 0 件) を経て、**Phase 2d-α 適用済 commit (`0587c574da`) の cold launch verify** を B 作業再開の first task として実施する。

**B 作業再開の地点** = pivot doc §3.5 (A) ルート採用 = Phase 2d-α 残置 + verify 実施 + 結果次第で Phase 2d-β (= 設計 source of truth に従う migration) へ進行。

---

## §0.5 pivot 〜 本 handoff までの経緯要約 (= 2026-06-03 session 末)

1. **2026-06-03 設計 phase**: η-28 UBO 全体設計 chapter 群 (01-10 + 06a/06a-prep/06b/06c) 起案完了 (= commit `1eaee2035e` 〜 `f67c68792b`)
2. **2026-06-03 第一次査読 + 修正**: 査読指摘 34 件全件修正 (commit `dfc5ad5aa2`)
3. **2026-06-03 第二次査読 report 起案** (commit `03a5817a09`): PASS-with-major-fixes 判定 + 致命傷候補 2 件 + 修正推奨 18 件 + AYA 判断仰ぎ新規 4 件
4. **2026-06-03 工程表反映** (commit `9b4033a735`): 00-charter §2 領域 4 + §5.3a 新設 / 04-frame-context §1.1 / §1.3 / §6.1 (UBO pivot 5 箇所追記)
5. **2026-06-03 過去 B 作業 audit** (commit 本 session): 3 Agent 並列で set=2 26 + set=3 32 + set=0 3 + set=1 2 = 63 UBO + sampler 16 個全件 audit、**整合率 100% / revert 必要 0 件**
6. **2026-06-03 Phase 2d-α 処遇判断確定**: pivot doc §3.5 (A) cold launch verify ルート採用 = 本 handoff 起案 (本 doc)

---

## §1 pre-requisite 必読 file (= 最低限 3 件、memory `feedback_handoff_minimal_pre_req_read` 準拠)

新規 session 開始時に **最低限 3 件のみ** を Read で先読み、追加は pinpoint Read で必要時に切替。

### §1.1 必読 1: 本 handoff (= 本 file)
- 全体把握 + verify 期待値 + 次 step

### §1.2 必読 2: audit 集約 doc
- `docs/specs/ayastorm-r41-gl-removal/audit-past-b-work-vs-design-2026-06-03.md`
- 過去 B 作業整合性 = 100%、revert 0 件、設計 chapter 群 source of truth 確定

### §1.3 必読 3: reference doc 該当章
- `docs/specs/ayastorm-r41-gl-removal/reference-shader-location-map.md` **§8 cookbook** (= verify 期待値 + parse fail 7→2 / link 0 / 5 program SPIR-V PASS 解説)

### §1.4 pinpoint Read (= verify 中に必要時のみ)
- `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-pivot-to-ubo-design.md` §4 (Phase 2d-α 適用済 5 file 編集内容)
- `design/05-existing-inventory-link.md` §3.3 (= 既存 26 UBO mapping)
- `design/09-phase-roadmap.md` §2 (= verify FAIL 時の乖離 audit 起点)

---

## §2 Phase 2d-α cold launch verify scope (= 本 session first task)

### §2.1 verify 対象 commit

| commit | 種別 | 内容 |
|---|---|---|
| `0587c574da` | feat | Phase 2d-α 5 GLSL file 適用 (pointLightF / spotLightF / pbrterrainF / pbrterrainUtilF / pbrTerrainBakeF) |
| `9a576884c0` | docs | reference doc 更新 (binding 10 note + η-28-E/F 範式追加) |

### §2.2 verify 期待値 (= reference doc §8 cookbook より)

| 観測項目 | 期待値 |
|---|---|
| **GLSL parse fail 件数** | 7 → **2** (= 5 file 修正で 5 fail 解消想定、残 2 fail は既知別件) |
| **link error 件数** | **0** (= UBO binding conflict 等の link 段階 error 無し) |
| **5 program SPIR-V 生成** | **PASS** (= pointLightF / spotLightF / pbrterrainF / pbrterrainUtilF / pbrTerrainBakeF の SPIR-V cross compile 成功) |
| **viewer 起動** | **黒画面 + UI 描画 PASS** (= vk-α 空転 baseline 維持) |
| **AYA cold launch ~10 分動作** | **sustained PASS** (= regression 0 件 = AYAstorm 機能維持) |

### §2.3 verify 手順 (= AYA 担当)

1. `git pull origin feature/ayastorm-r41-gl-removal` で最新取得 (本 session push 済)
2. AYAstorm ビルド手順 (memory `project_build_procedure`) = configure → build → install → cache clear
3. cold launch (= shader cache clear 後初回起動)
4. log 取得 (`~/.ayastorm_x64/logs/AYAstorm.log`)
5. log を Claude に渡す (= **log の grep/解析は Claude が直接実施、貼り付けさせない**、memory `feedback_log_reading`)

---

## §3 verify 結果ごとの次 step

### §3.1 PASS 時 (= 期待値全部達成)

→ **Phase 2d-α complete handoff 起草** + **Phase 2d-β prep 起案 (= 設計 source of truth に従う新規 migration の第一 phase)**:

- Phase 2d-β scope = `design/09-phase-roadmap.md` Phase 0 (計測) 着手 = setter 30 entry point 計測 + 既存 GL state cookbook 反映
- 計測は **docs 化 (= spec doc)**、`indra/` 改変は次々 phase (Phase 1+) に持越 (memory `feedback_design_phase_no_code_write` 範式継承)
- Phase 2d-β 着手前に **致命傷 2 件 AYA 判断 (Q22-NUM / Q23-K) + 新規判断仰ぎ 4 件 (Q22-NUM/Q23-K/Q24-S1/Q25-21CNT)** が前提

### §3.2 FAIL 時 (= 期待値乖離)

→ **乖離内容 audit**:

1. **parse fail > 2 件**: Phase 2d-α 修正 5 file の規約適用漏れ調査 (= η-28-E/F 範式の reference doc §6-E と照合)
2. **link error 発生**: UBO binding conflict 調査 (= chapter 06c §2 set=0/1/2/3 配線確定値と既存 binding 番号照合)
3. **5 program SPIR-V FAIL**: cross compile error 調査 (= glslang library 仕様と GLSL コード照合)
4. **viewer 起動 FAIL / 黒画面外**: 設計 chapter 群と既存 GL state cookbook の乖離 audit (= chapter 09 §3 Phase 0 計測手順前倒し)

→ 乖離点が **設計 chapter 群の欠落** 起源なら **設計 chapter 群修正 (= 第二次査読 §8 修正推奨 20 件 + 致命傷 2 + AYA 判断 4 へ追加登録)**、**Phase 2d-α GLSL 編集の bug** なら **修正 commit 追加**。

### §3.3 PASS 時の Phase 2d-β 着手判断 (= AYA review boundary)

Phase 2d-β prep 起草後、AYA review PASS で着手。本 handoff は **Phase 2d-α verify 完遂までの永続参照**、verify PASS 後は **Phase 2d-α complete handoff** に役割移管。

---

## §4 Phase 2d-α 適用済 5 file 編集内容 (= pivot doc §4.1 再掲、verify 時の照合 source)

| file | 編集内容 | 修正対象 Issue |
|---|---|---|
| `class3/deferred/pointLightF.glsl` | 自己 PerDrawUBO_LightParams 宣言削除 + body の `size` → `spot_light_size`、`color` → `spot_light_color` (Vulkan path のみ rename、`#ifdef LL_VULKAN_GLSL` 分岐) | Issue B (`size` undeclared at L2439 in transformed dump、η-28-E 範式) |
| `class3/deferred/spotLightF.glsl` | PerProgramUBO_SpotLightF に `vec3 center + float _pad_center` chunk 3 追加、`uniform vec3 center;` を `#ifndef LL_VULKAN_GLSL` で wrap、自己 PerDrawUBO_LightParams 削除、`color.rgb` → `spot_light_color.rgb` rename | Issue E + Issue F (MULTI_SPOTLIGHT permutation で `center` 参照、η-28-C type 3 範式) |
| `class1/deferred/pbrterrainF.glsl` | L47-51 `struct TerrainMix` を `#ifndef TERRAIN_MIX_DEFINED` guard wrap | Issue C (TerrainMix struct redefinition at L1750 in transformed dump、η-28-F 範式) |
| `class1/deferred/pbrterrainUtilF.glsl` | L183-187 同じ guard wrap | Issue C 連動 |
| `class1/interface/pbrTerrainBakeF.glsl` | L34 同じ guard wrap | 予防修正 (η-28-F 範式 future-proofing) |

---

## §5 残課題 (= verify と独立、AYA 判断 / main session 修正)

### §5.1 致命傷候補 2 件 AYA 判断待ち (= 第二次査読 §2.2 / §4.1)

| ID | 内容 |
|---|---|
| **Q22-NUM** | set=2 帯 25 vs 26 数値矛盾 (chapter 01/05/inventory:25 vs chapter 02:26、total 84 vs 85) |
| **Q23-K** | chapter 09 §2.1 K placeholder と §5.2 template 具体 phase 確定形矛盾 |

### §5.2 新規 AYA 判断仰ぎ 4 件 (= 第二次査読 §7)

| ID | 内容 |
|---|---|
| **Q22-NUM** | (= §5.1 と同一、chapter 10 §1 反映待ち) |
| **Q23-K** | (= §5.1 と同一、chapter 10 §1 反映待ち) |
| **Q24-S1** | chapter 06a §9 (S1) 持越 vs 06a-prep §2.2.2「(S1) 解消」の chapter 間矛盾 = getGlobalRegistry() 不存在確認済 |
| **Q25-21CNT** | chapter 10 §1 冒頭の AYA 判断仰ぎ 21 件 分類表 (chapter 別) 追加要否 |

### §5.3 設計 chapter 群 修正推奨 20 件 (= 第二次査読 §8.1 18 件 + 本 audit §3.1 2 件)

- 第二次査読 §8.1: chapter 01-1 + 02-2 + 05-3 + 06a-4 + 09-5 + 10-2 + inventory-1 + handoff-1 = 18 件
- 本 audit §3.1: Ch05 §3.3 25→26 (= Q22-NUM 解消方針追従) + `FrameAtmosphere_Lighting` → `FrameAtmosphere` rename (= `_Lighting` suffix 削除、chapter 02 §3.1 既存案準拠 = Wave G 反映済 2026-06-03) = 2 件
- = **計 20 件**、AYA 判断 4 件後に main session で実施予定

### §5.4 工程表追記 5 件 (= 本 session 実施済、commit `9b4033a735`)

= 00-charter §2 領域 4 + §5.3a / 04-frame-context §1.1 / §1.3 / §6.1 = 完了

### §5.5 修正 / 判断 総計

| 区分 | 件数 | 状態 |
|---|---|---|
| AYA 判断仰ぎ (致命傷 + 新規) | 4 件 | 待ち |
| 設計 chapter 群 修正推奨 | 20 件 | AYA 判断後実施 |
| 工程表追記 | 5 件 | **実施済** |
| **総計** | **29 件** | **5 件完了 / 24 件保留** |

---

## §6 関連 doc / commit / handoff cross-ref

### §6.1 関連 doc (= 本 handoff から参照)

- `audit-past-b-work-vs-design-2026-06-03.md` (= 本 session 起案、整合率 100% 確定 source)
- `design-review-2026-06-03-second-pass.md` (= 第二次査読 report、致命傷 2 + 修正推奨 20)
- `design/01-10 + 06a/06a-prep/06b/06c` (= 設計 source of truth)
- `reference-shader-location-map.md` §8 cookbook (= verify 期待値)
- `00-charter.md` §2 領域 4 + §5.3a (= 工程表反映済)
- `04-frame-context.md` §1.1 / §1.3 / §6.1 (= 工程表反映済)

### §6.2 本 session commit (= 2026-06-03)

| hash | 種別 | 内容 |
|---|---|---|
| `03a5817a09` | docs | 第二次査読 report 起案 (361 行) |
| `9b4033a735` | docs | UBO pivot 工程表反映 (00-charter + 04-frame-context、5 箇所) |
| (本 commit) | docs | audit 集約 doc 起案 + Phase 2d-α verify prep handoff 起案 |

### §6.3 関連 handoff (= 過去 → 本 handoff)

- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-pivot-to-ubo-design.md` (= 設計 pivot、本 handoff の direct 前任)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-prep.md` (= Phase 2d-α 適用前 prep、pivot 直前)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-review.md` (= 第一次査読 handoff)

### §6.4 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` (= r41 milestone active)
- `project_ayastorm_r41_design_principles.md` (= 2 大設計原則 = call site 温存 + Core 分散)
- `feedback_ubo_migration_one_at_a_time.md` (= 1 UBO ずつ migration、本 handoff 後の Phase 2d-β 以降適用)
- `feedback_design_phase_no_code_write` (= design-phase 中は indra/ 改変禁止、Phase 0 計測も doc 化)
- `feedback_handoff_minimal_pre_req_read` (= 必読 3 件 + pinpoint Read で context 圧迫回避)
- `feedback_log_reading` (= log 解析は Claude 自身で実施、AYA に貼り付けさせない)

---

## §7 本 handoff の制約 (= 次 session 開始時に Claude が忘れがちな点)

- **Phase 2d-α verify は B 作業再開の first task** であり、Phase 2d-β 着手前提条件 (memory `feedback_doubt_self_first` で「verify 飛ばして次行く」を抑止)
- **verify PASS でも AYA 判断 4 件 + 修正 20 件は別途残課題**、Phase 2d-β 着手前に AYA に判断仰ぎ
- **verify FAIL 時の対応** = 安易に「Phase 2d-α revert」に走らず、乖離 audit で **設計 chapter 群欠落起源 か GLSL bug か** を判定 (memory `feedback_render_full_trace_first` 範式)
- **memory `feedback_no_scope_shrink`**: AYA verify 結果対応の literal scope を「parse fail 数のみ」に縮小しない、link / SPIR-V / 起動 / sustained 全項目確認
- **設計 chapter 群 18 件修正は AYA 判断待ち** = 本 handoff の verify 結果と独立、Phase 2d-α verify を理由に「先に修正」しない

---

**本 handoff は Phase 2d-α verify 完遂までの永続参照**。verify PASS 後は `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-complete.md` に役割移管、verify FAIL 時は乖離 audit 結果を本 handoff §3.2 に追記して継続使用。
